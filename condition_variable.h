#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <iostream>

std::mutex mut;
bool ready = false;
std::string data = "";
std::condition_variable cond_var;

void consumer()
{
	std::unique_lock lock(mut);
	cond_var.wait(lock, [&] { return ready; });
	std::cout << data; // prints "some data"
}

void equivalent_consumer()
{
	std::unique_lock lock(mut);
	while (!ready)				// blocks processing ...
	{
		cond_var.wait(lock);	// ... if this wakes spuriously
	}
	std::cout << data; // prints "some data"
}

void producer()
{
	data = "some data";
	{
		std::scoped_lock(mut);
		ready = true;
	}
	cond_var.notify_one();
}