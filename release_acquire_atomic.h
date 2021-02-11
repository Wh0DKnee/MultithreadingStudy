#pragma once
#include <atomic>

std::atomic<bool> x = false, y = false;
std::atomic<int> z = 0;

void write_x_then_y() // thread1
{
	x.store(true, std::memory_order_relaxed);	// happens before *1
	y.store(true, std::memory_order_release);	// synchronizes with *2
}
void read_y_then_x() // thread2
{
	while (!y.load(std::memory_order_acquire));	// *2 this! Because we spin until we read the value written by thread1, the
												// synchronizes-with relationship is only established if we've read the written value!

	if (x.load(std::memory_order_relaxed))		// *1 this! Guaranteed to succeed! Without synchronization, the if check could fail, because
												// we might see the update to y before the update to x.
		++z; 
}

void read_y_then_x_if() // thread2 (alternative)
{
	if (y.load(std::memory_order_acquire))		
	{
		// synchronizes-with relation established here, because we've read the value
		// written to y by thread1
		if (x.load(std::memory_order_relaxed)) // guaranteed to pass
			++z;
	}
	else
	{
		// no synchronizes-with relation here!
	}

}