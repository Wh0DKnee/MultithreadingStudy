#pragma once
#include <atomic>
#include <cassert>

std::atomic_int a = 0;
std::atomic_int b = 0;

void thread1()
{
	a.store(1, std::memory_order_relaxed);		// write 1 to a in thread1's cache
	int c = b.load(std::memory_order_relaxed);	
	
	// write a back to main memory
}

void thread2()
{
	b.store(1, std::memory_order_relaxed);		// write 1 to b in thread2's cache
	int d = a.load(std::memory_order_relaxed);

	// write b back to main memory
}

/*
Which values are possible for c and d?

a) c == 0 and d == 0
b) c == 1 and d == 0
c) c == 0 and d == 1
d) c == 1 and d == 0

Intuitively, you would think only b, c, and d are possible. However,
a is possible as well! Without compile time and runtime reordering,
a would be impossible, because at least one of the stores must happen
before any load. Let's assume there is no compile time reordering.
a is still possible, because the store of thread1 might not be visible
to thread2 at the time of the load and vice versa, because the write 
on a might not have made it back to main memory yet.
*/