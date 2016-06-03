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
	int j = 0;
	for (int i = 0;i < 10000000;i++)
		j = j + rand() % 10;
	std::cout << j << std::endl;
	return nullptr;
}


int main() {
	
	go::goinit();
	for (int i = 0;i < 30;i++) {
		auto g = new go::G;
		g->func = job;
		g->args = new std::map<std::string, void*>();
		go::gocommit(g);
	}
	//while (1);
	go::goend();
	/*
	for (int i = 0;i < 30;i++) {
		std::thread t(job, std::map<std::string, void*>());
		t.detach();
	}
	while (1);*/
}