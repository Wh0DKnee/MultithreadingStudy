#pragma once
#include <atomic>

std::atomic<bool> x, y;
std::atomic<int> z;

// release fence = load-store + store-store
// acquire fence = load-store + load-load

void write_x_then_y()
{
	x.store(true, std::memory_order_relaxed);
	std::atomic_thread_fence(std::memory_order_release);	// can think of this as a "git push" to main memory (sort of?),
															// so x is in main memory now
	y.store(true, std::memory_order_relaxed);
}
void read_y_then_x()
{
	while (!y.load(std::memory_order_relaxed));
	std::atomic_thread_fence(std::memory_order_acquire);	// can think of this as a "git pull" from main memory (sort of?),
															// 
	if (x.load(std::memory_order_relaxed))
		++z;
}