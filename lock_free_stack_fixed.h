#pragma once
#include <atomic>
#include <memory>

/*
	Downside:
	If the load is high and there is never a quiet period where
	pop() isn't called frequently, the to_be_deleted list could
	grow indefinitely.
*/

template <typename T>
class lock_free_stack_fixed
{
private:

	struct node
	{
		std::shared_ptr<T> data;
		node* next;
		node(T const& data_) : data(std::make_shared(data_)) {}
	};

	std::atomic<node*> head;
	std::atomic<unsigned> threads_in_pop;
	std::atomic<node*> to_be_deleted;

	void try_reclaim(node* old_head)
	{
		if (threads_in_pop == 1)
		{
			// lets say this thread is thread 0.
			// of course, threads_in_pop may already be higher here because thread 1 may
			// call pop, but that doesn't matter, because if thread 1 calls it after we've
			// checked threads_in_pop == 1, then thread 0 has already updated the stack's head
			// and thread 1 will not read the same value for old_head in pop as thread 0.
			// Therefore, if we've made it here ...
			node* nodes_to_delete = to_be_deleted.exchange(nullptr);
			if (!--threads_in_pop)
			{
				// similar argumentation as above, since we've set to_be_deleted
				// to nullptr and stored the previous value in nodes_to_delete,
				// other threads may still need old_heads value's, but they call pop
				// after we've made the if check above and will therefore start a new
				// old_heads list in to_be_deleted. They need no nodes from nodes_to_delete.
				// So, we can safely delete them here.
				delete_nodes(nodes_to_delete);
			}
			else if (nodes_to_delete)
			{
				// other threads have entered pop() in the meantime and therefore may have
				// modified the to_be_deleted list (in the else branch below) before thread 0
				// has read the value into nodes_to_delete. Therefore, some nodes in the 
				// nodes_to_delete list are still in use and must therefore be returned to
				// the nodes_to_delete list.
				chain_pending_nodes(nodes_to_delete);
			}
			delete old_head; // ... we can safely delete the old_head.
		}
		else
		{
			chain_pending_node(old_head);
			--threads_in_pop;
		}
	}

	void chain_pending_nodes(node* nodes)
	{
		node* last = nodes;
		while (node* const next = last->next)
		{
			last = next;
		}
		chain_pending_nodes(nodes, last);
	}

	void chain_pending_nodes(node* first, node* last)
	{
		last->next = to_be_deleted;
		while (!to_be_deleted.compare_exchange_weak(last->next, first));
	}

	void chain_pending_node(node* n)
	{
		chain_pending_nodes(n, n);
	}

	static void delete_nodes(node* nodes)
	{
		while (nodes)
		{
			node* next = nodes->next;
			delete nodes;
			nodes = next;
		}
	}

public:

	void push(T const& data)
	{
		node* const new_node = new node(data);
		new_node->next = head.load();
		while (!head.compare_exchange_weak(new_node->next, new_node)) { ; }
	}

	std::shared_ptr<T> pop()
	{
		++threads_in_pop;
		node* old_head = head.load();
		while (old_head && !head.compare_exchange_weak(old_head, old_head->next)) { ; }
		std::shared_ptr<T> res;
		if (old_head)
		{
			res->data = std::move(old_head->data());
			try_reclaim(old_head);
		}
		return res;
	}
};