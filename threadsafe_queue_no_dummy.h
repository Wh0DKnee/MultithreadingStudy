#pragma once
#include <memory>
#include <mutex>

template <typename T>
class threadsafe_queue_no_dummy
{
public:

	threadsafe_queue_no_dummy() {}
	threadsafe_queue_no_dummy(threadsafe_queue_no_dummy& other) = delete;
	threadsafe_queue_no_dummy& operator=(threadsafe_queue_no_dummy& other) = delete;

	void push(T val);
	std::shared_ptr<T> try_pop();

private:

	struct node
	{
		node(T val) : data(val), next(nullptr) {}
		T data;
		std::unique_ptr<node> next;
	};

	mutable std::mutex head_mut;
	mutable std::mutex tail_mut;

	std::unique_ptr<node> head;
	node* tail;
};

template<typename T>
void threadsafe_queue_no_dummy<T>::push(T val)
{
	std::unique_ptr<node> p(new node(std::move(val));
	node* const new_tail = p.get();
	std::scoped_lock tail_lock(tail_mut);
	if (tail)
	{
		tail->next = std::move(p);
	}
	else
	{
		std::scoped_lock head_lock(head_mut);
		head = std::move(p);
	}
	tail = new_tail;
}

template<typename T>
std::shared_ptr<T> threadsafe_queue_no_dummy<T>::try_pop()
{
	std::scoped_lock head_lock(head_mut);
	if (!head)
	{
		return nullptr;
	}

	std::shared_ptr res = std::make_shared<T>(std::move(head->data));
	std::unique_ptr<node> old_head(std::move(head));
	head = std::move(old_head->next);
	if(!head)
	{
		// PROBLEM: If the queue has one element and another thread calls push in the meantime, we are in big trouble.
		// Here, tail still points to the element (which is freed after this function). If push is called from another
		// thread, we point this element's next pointer to the newly pushed element. After this function returns, the
		// element will be deleted (because only old_head points to it and old_head goes out of scope), and therefore the 
		// newly pushed element will also be deleted, because only the deleted element's next pointer pointed to it.
		// To circumvent this issue, we can add a dummy node so that head and tail will only ever point to the same
		// node if the queue is empty. See "threadsafe_queue" for the implementation.
		std::scoped_lock tail_lock(tail_mut);
		tail = nullptr;
	}
	return res;
}
