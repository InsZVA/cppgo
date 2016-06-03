#include <thread>
#include "go.h"
#include "windows.h"

int i = 0;
std::mutex m;

void* job(go::args_type args) {
	std::cout << "jobjobjobjobjobjobjobjobjobjobjobjobjobjobjobjob" << std::endl;
	m.lock();
		i++;
	m.unlock();
	Sleep(200);
	return nullptr;
}

int main() {
	go::goinit();
	for (int i = 0;i < 300;i++) {
		auto g = new go::G;
		g->func = job;
		g->args = new std::map<std::string, void*>();
		go::gocommit(g);
	}
	while (1);
}