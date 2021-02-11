#pragma once
#include <atomic>
#include <cassert>

std::atomic_int a = 0;
std::atomic_int b = 1;

void thread1()
{
	a.store(1, std::memory_order_relaxed);
	b.store(0, std::memory_order_relaxed);

	/*compiler is free to swap these two lines:
	* 
	* b.store(0, std::memory_order_relaxed);
	* a.store(1, std::memory_order_relaxed);
	* 
	* as this would not change the behavior of
	* a single threaded program - line 10 and 11
	* have no side effect on each other!		
	*/
}

void thread2()
{
	if (b.load(std::memory_order_relaxed) == 0)
	{
		assert(a == 1);
		/*can fire! if compiler decides to rearrange
		* the operations in thread1, b can be 0 before
		* a is 1. (this can also happen without compile
		* time reordering, due to runtime reordering:
		* thread1 may store a first and b second, but this
		* only updates the values in thread1's local cache.
		* whether the update to a or the update to b is visible
		* to the main memory first is not known, so even without
		* compile time reordering, it's possible that the update
		* to b is visible to thread2 before the update to a.)
		*/
	}
}