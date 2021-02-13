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

		interested[me].store(true, std::memory_order_relaxed);	// let's call this "A-write"
		turn.exchange(he, std::memory_order_acq_rel);			// let's call this "B"

		while (interested[he].load(std::memory_order_acquire)	// let's call this "A-load"
			&& turn.load(std::memory_order_relaxed) == he)
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

	Thread 0 calls lock. It writes true to interested[0] with relaxed ordering and exchanges
	0 in turn with 1 with acq-rel ordering. The while condition fails because interested[he]
	is still at it's initial value false and thread 0 has successfully acquired the lock.

	Now, thread 1 calls lock. It writes true to interested[1] and exchanges 1 in turn with 0 with
	acq-rel ordering:

	If the value read is 1, then the write at "B" from thread 0 synchronizes with 
	the read at "B" from thread	1. Since "A-write" is sequenced-before "B" and "B" is sequenced-before 
	"A-load", "A-write" happens-before "A-load". Thread 1 is guaranteed to see the interested[0] == true
	at "A-load" and will therefore not acquire the lock.

	If the value read is 0, then what???

	*/
};