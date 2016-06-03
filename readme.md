#CppGo

## Destination

Make cpp can use goroutine, which is a light thread well scheduled that both parralization and power-saving has been thoughted.

## Compare

Compare to use std::thread directly:

![std::thread * 30](1.png)  
std::thread * 30 

![go::gocommit * 30](2.png)  
go::commit * 30

Intel i7 - 4712MQ 8Cores

## Feather

Compared to use thread whenever there is a job, it will cost less to switch between threads (because there are at most cores number of 
working thread in cppgo); Compared to use thread pool, it can mostly parralize when a specify thread block (the context P will move to 
another working thread).