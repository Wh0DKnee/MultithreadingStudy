#pragma once
#include <memory>
#include <atomic>

// single producer single consumer lock free queue

template <typename T>
class lock_free_queue_spsc
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		node* next;
		node() : next(nullptr) {}
	};
	std::atomic<node*> head;
	std::atomic<node*> tail;
	node* pop_head();

public:
	lock_free_queue_spsc() : head(new node), tail(head.load()) {}
	lock_free_queue_spsc(const lock_free_queue_spsc& other) = delete;
	lock_free_queue_spsc& operator=(const lock_free_queue_spsc& other) = delete;
	~lock_free_queue_spsc();
	
	void pop(std::shared_ptr<T>& out_val);
	void push(T val);
};

template<typename T>
typename lock_free_queue_spsc<T>::node* lock_free_queue_spsc<T>::pop_head()
{
	node* old_head = head.load();
	if (old_head == tail.load())
	{
		return nullptr;
	}
	head.store(old_head->next);
	return old_head;
}

template<typename T>
lock_free_queue_spsc<T>::~lock_free_queue_spsc()
{

	while (node* node = head.load())
	{
		head.store(node->next);
		delete node;
	}
}

template<typename T>
void lock_free_queue_spsc<T>::pop(std::shared_ptr<T>& out_val)
{
	node* old_head = pop_head();
	if (!old_head)
	{
		out_val = nullptr;
		return;
	}
	out_val = old_head->data;
	delete old_head;
}

template<typename T>
void lock_free_queue_spsc<T>::push(T val)
{
	node* new_dummy = new node();
	node* old_tail = tail.load();
	old_tail->data = std::make_shared(val);
	old_tail->next = new_dummy;
	tail.store(new_dummy);
}
