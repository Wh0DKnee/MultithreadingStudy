#pragma once
#include <atomic>
#include <memory>

template <typename T>
class lock_free_stack_with_memory_leak
{
private:
	
	struct node
	{
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) : data(std::make_shared(data_)) {}
	};

	std::atomic<node*> head;

public:

	void push(T const& data)
	{
		node* const new_node = new node(data);
		new_node->next = head.load();
		// A
		while (!head.compare_exchange_weak(new_node->next, new_node)) { ; }
		//                                      ^             ^   
		//                                   expected      desired
		//
		// If thread 0 gets to A and thread 1 calls push and finishes it before
		// thread 0 continues, new_node->next will point to an out-of-date head.
		// This is not a problem though, because then compare_exchange will fail
		// because head is not equal to the expected value. At the same time,
		// new_node->next will be updated to the correct new head value and will
		// succeed in the next iteration (unless another thread calls push and finishes
		// before thread 0 again, in which case we'll just keep iterating until
		// we succeed.
	}

	std::shared_ptr<T> pop()
	{
		node* old_head = head.load();
		// B
		while (old_head && !head.compare_exchange_weak(old_head, old_head->next)) { ; }
		result = old_head ? old_head->data : nullptr;
		// delete old_head
		// If we delete the old_head, we are in trouble. Imagine thread 0 makes it to B.
		// Then, thread 1 calls pop and finishes, deleting old_head. thread 0 now has a
		// dangling pointer in old_head. A "solution" is to not delete old_head and creating
		// a memory leak. Now thread 0 still points to a valid node, and if the head has
		// been modified in the meantime by thread 1, the compare_exchange will assign old_head
		// to the updated head and we're all good (except for the memory leak lol).
		// For a solution to this problem, see lock_free_stack_fixed.h
	}
};
