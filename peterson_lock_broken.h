#pragma once
#include <atomic>
#include <thread>

class peterson_lock_broken
{
private:
	// indexed by thread ID, 0 or 1
	std::atomic<bool> interested[2];
	// who's yielding priority?
	std::atomic<int> turn;

	int threadID()
	{
		// let's assume this is either 0 or 1.
		return std::hash<std::thread::id>{}(std::this_thread::get_id());
	}

public:

	peterson_lock_broken()
	{
		turn.store(0, std::memory_order_release);
		interested[0].store(false, std::memory_order_release);
		interested[1].store(false, std::memory_order_release);
	}

	void lock()
	{
		int me = threadID(); // either 0 or 1
		int he = 1 - me; // the other thread

		interested[me].exchange(true, std::memory_order_acq_rel); // let's call this "A-write"
		turn.store(me, std::memory_order_release);

		while (interested[he].load(std::memory_order_acquire) // let's call this "A-load"
			&& turn.load(std::memory_order_acquire) == me)
		{
			continue; // spin
		}
	}

	void unlock()
	{
		int me = threadID();
		interested[me].store(false, std::memory_order_release);
	}

	/*
	Example:

	Thread 0 calls lock. It writes true to interested[0] with acq_rel ordering and
	writes 0 to turn with release ordering. The while condition fails because interested[he]
	is still at it's initial value false and thread 0 has successfully acquired the lock.

	Now, thread 1 calls lock. It writes true to interested[1] and writes 1 to turn. Now it 
	reads interested[0] in the while check. The question is: Is there any synchronization that
	guarantees thread 1 to see the value written by thread 0? We need a inter-thread happens-before
	relationship between thread0's "A-write" and thread1's "A-read". We get this, if thread 1 reads a value
	with acquire ordering that thread 0 has written with release ordering, and if that read in thread
	1 is sequenced-before "A-load", and if "A-write" is sequenced-before that write in thread 0. 

	In other words: Is there a line in the code before "A-load" that reads a value written by thread 0
	in a line after "A-write"?
	There isn't. Thread 1 may or may not see the value written to interested[0] and therefore it can happen
	that thread 1 acquires the lock while thread 0 holds it.
	*/
};