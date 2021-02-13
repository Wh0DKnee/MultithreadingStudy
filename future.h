#pragma once
#include <future>
#include <atomic>
#include <iostream>

void example()
{
	std::future future = std::async(std::launch::async, [] { return "hello"; });

	std::cout << future.get(); // prints "hello"
}