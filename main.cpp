#include <iostream>
#include <atomic>
#include <thread>

std::atomic<bool> flag(false);
int a = 0;

void func1()
{
    a = 100;
    atomic_thread_fence(std::memory_order_release);
    flag.store(true, std::memory_order_relaxed); 

    // when func2 sees this change, it is guaranteed to also see a = 100 thanks to the fence
    // without the fence, the compiler would be free to reoder the instructions to:
    //      flag.store(true);
    //      a = 100;
    // and func2 could read the old value of a (or something in between, as a data race could happen)
}

void func2()
{
    while (!flag.load(std::memory_order_relaxed))
        ;

    atomic_thread_fence(std::memory_order_acquire);
    std::cout << a << '\n'; // guaranteed to print 100
}

int main()
{
    std::thread t1(func1);
    std::thread t2(func2);

    t1.join(); t2.join();

}