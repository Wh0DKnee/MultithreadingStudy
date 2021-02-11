#pragma once
#include <memory>
#include <mutex>

/* example usage:
threadsafe_lut<int, std::string> lut;
std::thread t0(&threadsafe_lut<int, std::string>::add_or_update_mapping, &lut, 6, "six");
std::thread t1(&threadsafe_lut<int, std::string>::add_or_update_mapping, &lut, 7, "seven");
std::thread t2(&threadsafe_lut<int, std::string>::add_or_update_mapping, &lut, 8, "eight");
std::thread t3(&threadsafe_lut<int, std::string>::add_or_update_mapping, &lut, 9, "nine");

t0.join();
t1.join();
t2.join();
t3.join();

std::string s;
std::thread t4(&threadsafe_lut<int, std::string>::remove_mapping, &lut, 9);
using overload_type = bool (threadsafe_lut<int, std::string>::*)(int, std::string&);
overload_type value_for = &threadsafe_lut<int, std::string>::value_for;
std::thread t5(value_for, &lut, 6, std::ref(s));

t4.join();
t5.join();

std::cout << s << std::endl;

std::thread t6(value_for, &lut, 9, std::ref(s));

t6.join();
std::cout << s << std::endl;
*/

template<typename T>
class threadsafe_queue
{
public:
	threadsafe_queue() : head(std::make_unique<node>()), tail(head.get()) {}

	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;

	std::shared_ptr<T> try_pop();
	void push(T new_value);

private:

	std::mutex head_mutex;
	std::mutex tail_mutex;

	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;

		node(T data_) : data(std::move(data_)) {}
		node() = default;
	};

	node* get_tail();
	std::unique_ptr<node> pop_head();

	std::unique_ptr<node> head;
	node* tail;
};

template<typename T>
inline std::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
	auto old_head = pop_head();
	return old_head ? old_head->data : nullptr; // no lock required anymore, node is already removed from data structure
}

template<typename T>
inline void threadsafe_queue<T>::push(T new_value)
{
	auto p = std::make_unique<node>();						// new dummy node
	auto data = std::make_shared<T>(std::move(new_value));
	node* new_tail = p.get();
	std::lock_guard<std::mutex> tail_lock(tail_mutex);
	tail->data = data;										// move data into previous dummy node
	tail->next = std::move(p);
	tail = new_tail;
}

template<typename T>
inline typename threadsafe_queue<T>::node* threadsafe_queue<T>::get_tail()
{
	std::lock_guard<std::mutex> tail_lock(tail_mutex);
	return tail;
}

template<typename T>
inline std::unique_ptr<typename threadsafe_queue<T>::node> threadsafe_queue<T>::pop_head()
{
	std::lock_guard<std::mutex> head_lock(head_mutex);
	if (head.get() == get_tail())
	{
		return nullptr;
	}
	auto old_head = std::move(head);
	head = std::move(old_head->next);
	return old_head;
}
