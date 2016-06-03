#include "go.h"

namespace go {
	volatile unsigned int nmspinning = 0;
	RWMutex nmspinning_lock;
	const unsigned int MAX_PROCS = 8;
	unsigned newP = 0;
	unsigned newM = 0;

	Queue<P*> gfpqueue;
	Queue<std::thread*> tqueue;

	std::vector<P*> gplist;
	//std::mutex gplist_mutex;
	//std::unique_lock<std::mutex> gplist_lock(gplist_mutex);
	std::mutex gplist_lock;

	template <class T> void debugPrint(T t) {
#ifdef DEBUG	//Because the empty function will be optimized by compiler
		std::cout << t << std::endl;
#endif
	}

	void addnmspinning() {
		nmspinning_lock.wLock();
		nmspinning++;
		nmspinning_lock.wUnlock();
	}

	void minusnmspinning() {
		nmspinning_lock.wLock();
		nmspinning--;
		nmspinning_lock.wUnlock();
	}

	bool steal(P* p, P* p2) {
		auto ng = p2->gqueue.queue.size();
		if (ng == 0)
			return false;
		else {
			ng = p2->gqueue.queue.size();
			if (ng == 0)
				return false;
			for (unsigned i = 0;i <= ng / 2;i++)
			{
				p->gqueue.push(p2->gqueue.get());
			}
		}
		debugPrint("steal success");
		return true;
	}

	void spin(M* m) {

		debugPrint("spinning...");
		addnmspinning();
		nmspinning_lock.rLock();
		while (nmspinning == 1)
		{
			nmspinning_lock.rUnLock();	//spinning
			nmspinning_lock.rLock();
		}
		nmspinning_lock.rUnLock();
		minusnmspinning();
		debugPrint("a m spin -> nonspin");
		while (1) {
			start_while:
			if (m->g != nullptr) {
				debugPrint("working...");
				m->g->func(*(m->g->args));
				delete m->g;
				m->g = nullptr;
				continue;
			}
			if (m->p == nullptr) {
				debugPrint("find a new p to m");
				auto nfp = gfpqueue.queue.size();
				if (nfp == 0) {
					debugPrint("a m go park");
					delete m;
					break;
				}
				else {
					auto p = gfpqueue.get();
					m->p = p;
					p->free = false;
					continue;
				}
			}
			if (m->g == nullptr) {
				debugPrint("find a new g to m");
				auto ng = m->p->gqueue.queue.size();
				if (ng == 0) {
					auto np = gplist.size();
					if (np == 0) {
						m->p = nullptr;
						delete m;
						debugPrint("a m go park");
						break;
					};
					auto i = rand() % np;
					if (steal(m->p, gplist[i])) {
						continue;
					}
					else {
						for (int j = 0;j < gplist.size();j++) {
							i = (i + 1) % gplist.size();
							if (steal(m->p, gplist[i])) {
								goto start_while;
							}
						}
						
						//gfpqueue.push(m->p);
						gfpqueue.lock.lock();
						gfpqueue.queue.push(m->p);
						m->p->free = true;
						gfpqueue.lock.unlock();

						debugPrint("p steal faild and go free");
						m->p = nullptr;
						delete m;
						debugPrint("a m go park");
						break;
					}
				}
				else {
					auto g = m->p->gqueue.get();
					m->g = g;
					continue;
				}
			}
		}
		
		
	}

	void gocommit(G* g) {
		debugPrint("new g commit");
		auto np = gplist.size();
		P* p = nullptr;
		if (np == 0) {
			p = new P;
			newP++;
			debugPrint("new p");
			gplist_lock.lock();
			gplist.push_back(p);
			gplist_lock.unlock();
			p->gqueue.push(g);
			gfpqueue.push(p);
			debugPrint("a p go to gfpqueue");
		}
		else {
			//TODO find a runnable p
			auto np = gplist.size();
			auto i = rand() % np;
			while (gplist[i]->free)
				i = (i + 1) % gplist.size();
			gplist[i]->gqueue.queue.push(g);
		}
		np = gplist.size();
		if (np < MAX_PROCS && nmspinning == 1) {
			auto m = new M;
			newM++;
			debugPrint("new m");
			if (p == nullptr) {
				m->p = new P;
				newP++;
			}
			else if (gfpqueue.queue.size() == 0) {
				m->p = new P;
				newP++;
			}
			else {
				m->p = gfpqueue.get();
			}
			debugPrint("new p");
			gplist_lock.lock();
			gplist.push_back(m->p);
			gplist_lock.unlock();
			auto t = new std::thread (spin, m);
			//t->detach();
			tqueue.push(t);
		}
	}

	void goinit() {
		auto m = new M;
		newM++;
		debugPrint("new m");
		auto t = new std::thread(spin, m);
		//t->detach();
		tqueue.push(t);
	}

	void goend() {
		while (!tqueue.queue.empty()) {
			auto t = tqueue.get();
			t->join();
		}
	}
}