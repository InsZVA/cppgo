#pragma once
#include <functional>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <cmath>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>

namespace go {

	class RWMutex {
	private:
		
		std::mutex wm, rmm;
		std::unique_lock<std::mutex> rmtx;
		std::condition_variable cond;
		int rd_cnt = 0;	//wait for read
		bool wr_cnt = false;	//wait for write
	public:
		RWMutex() : rmtx(rmm, std::defer_lock) {}
		void rLock() {
			while (wr_cnt)
				cond.wait(rmtx);
		}

		void rUnLock() {
			if (!wr_cnt)
				cond.notify_all();
		}

		void wLock() {
 			wm.lock();
			while (wr_cnt)
				cond.wait(rmtx);
			wr_cnt = true;
			wm.unlock();
		}

		void wUnlock() {
			wm.lock();
			wr_cnt = false;
			cond.notify_all();
			wm.unlock();
		}
	};

	template <class T> struct Queue {
		std::queue<T> queue;
		std::mutex mutex;
		std::unique_lock<std::mutex> lock;
		//std::mutex lock;
		Queue():lock(mutex, std::defer_lock){}
		void push(T t) {
			lock.lock();
			queue.push(t);
			lock.unlock();
		}
		T get() {
			auto ret = queue.front();
			lock.lock();
			queue.pop();
			lock.unlock();
			return ret;
		}
	};

	template <class S, class T> struct Map {
		std::map<S, T> map;
		RWMutex mutex;
		void set(S s, T t) {
			mutex.wLock();
			map[s] = t;
			mutex.wUnlock();
		}
		T get(S s) {
			mutex.rLock();
			auto ret = map[s];
			mutex.rUnLock();
			return ret;
		}
		void remove(S s) {
			mutex.wLock();
			map.erase(s);
			mutex.wUnlock();
		}
		unsigned int size() {
			mutex.rLock();
			auto ret = map.size();
			mutex.rUnLock();
			return ret;
		}
	};

	typedef std::map<std::string, void*> args_type;
	typedef void* (*func_type)(args_type args);
	

	typedef struct _g{
		func_type func = nullptr;
		args_type* args = nullptr;
	} G;

	typedef struct _p {
		Queue<G*> gqueue;
		bool free = false;
	} P;

	typedef struct _m {
		P* p = nullptr;
		G* g = nullptr;
	} M;

	void gocommit(G* g);
	void goinit();
	void spin(M* m);
	bool steal(P* p, P* p2);
	void minusnmspinning();
	void addnmspinning();
	void goend();
}
