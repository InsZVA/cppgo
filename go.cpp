#include "go.h"
#define DEBUG
namespace go {
	volatile unsigned int nmspinning = 0;
	RWMutex nmspinning_lock;
	const unsigned int MAX_PROCS = 8;

	Map<std::thread::id, M*> mmap;
	std::mutex mmap_lock;

	Queue<P*> gfpqueue;

	std::vector<P*> gplist;
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
		auto ng = p2->gqueue.gqueue.size();
		if (ng == 0)
			return false;
		else {
			ng = p2->gqueue.gqueue.size();
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
		mmap_lock.lock();
		mmap.set(std::this_thread::get_id(), m);
		mmap_lock.unlock();

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
			if (m->g != nullptr) {
				debugPrint("working...");
				m->g->func(*(m->g->args));
				delete m->g;
				m->g = nullptr;
				continue;
			}
			if (m->p == nullptr) {
				debugPrint("find a new p to m");
				auto nfp = gfpqueue.gqueue.size();
				if (nfp == 0) {
					debugPrint("a m go park");
					delete m;
					mmap.remove(std::this_thread::get_id());
					break;
				}
				else {
					auto p = gfpqueue.get();
					m->p = p;
					continue;
				}
			}
			if (m->g == nullptr) {
				debugPrint("find a new g to m");
				auto ng = m->p->gqueue.gqueue.size();
				if (ng == 0) {
					auto i = rand() % gplist.size();
					if (steal(m->p, gplist[i])) {
						continue;
					}
					else {
						for (int j = 0;j < gplist.size();j++) {
							i = (i + 1) % gplist.size();
							if (steal(m->p, gplist[i])) {
								continue;
							}
						}
						
						gfpqueue.push(m->p);
						debugPrint("p steal faild and go free");
						m->p = nullptr;
						delete m;
						mmap.remove(std::this_thread::get_id());
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
			debugPrint("new p");
			gplist_lock.lock();
			gplist.push_back(p);
			gplist_lock.unlock();
			p->gqueue.push(g);
			gfpqueue.push(p);
			debugPrint("a p go to gfpqueue");
		}
		else {
			auto m = mmap.get(std::this_thread::get_id());
			P* p;
			if (m == nullptr) {	//Main thread
				auto i = rand() % np;
				p = gplist[i];
			}
			else {
				p = m->p;
			}
			p->gqueue.push(g);
		}
		np = gplist.size();
		if (np < MAX_PROCS && nmspinning == 1) {
			auto m = new M;
			debugPrint("new m");
			if (p == nullptr)
				m->p = new P;
			else if (gfpqueue.gqueue.size() == 0) {
				m->p = new P;
			}
			else {
				m->p = gfpqueue.get();
			}
			debugPrint("new p");
			gplist_lock.lock();
			gplist.push_back(m->p);
			gplist_lock.unlock();
			std::thread t(spin, m);
			t.detach();
		}
	}

	void goinit() {
		auto m = new M;
		debugPrint("new m");
		std::thread t(spin, m);
		t.detach();
	}
}