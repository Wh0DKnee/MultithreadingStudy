#pragma once
#include <cassert>
#include <functional>
#include <type_traits>
#include <vector>
#include <utility>
#include <algorithm>
#include <shared_mutex>
#include <memory>

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

template<typename KeyType, typename ValueType>
class threadsafe_lut
{
	using HashEntry = std::pair<KeyType, ValueType>;
	using Bucket = std::vector<HashEntry>;
	using BucketIterator = typename Bucket::iterator;
	static_assert(std::is_default_constructible<std::hash<KeyType>>::value); // assert that KeyType is hashable

public:

	threadsafe_lut(size_t num_buckets = 19) : data(num_buckets), bucket_mutexes(num_buckets) {}

	void add_or_update_mapping(KeyType key, ValueType val);

	void remove_mapping(KeyType key);

	std::shared_ptr<ValueType> value_for(KeyType key);

	bool value_for(KeyType key, ValueType& out_val);

private:

	std::vector<Bucket> data;

	std::vector<std::shared_mutex> bucket_mutexes;

	BucketIterator find_in_bucket(Bucket& bucket, const KeyType& key)
	{
		BucketIterator pos = std::find_if(bucket.begin(), bucket.end(), [&](const HashEntry& entry) { return entry.first == key; });
		return pos;
	}

	size_t hash(const KeyType& key) const
	{
		return std::hash<KeyType>{}(key) % data.size();
	}

	Bucket& get_bucket(const KeyType& key)
	{
		return data[hash(key)];
	}
};

template<typename KeyType, typename ValueType>
inline void threadsafe_lut<KeyType, ValueType>::add_or_update_mapping(KeyType key, ValueType val)
{
	std::unique_lock lock(bucket_mutexes[hash(key)]);
	Bucket& bucket = get_bucket(key);
	auto pos = find_in_bucket(bucket, key);
	if (pos != bucket.end()) // found entry with same key
	{
		pos->second = val; // update entry
	}
	else // didn't find entry, so add new one
	{
		bucket.push_back(std::make_pair(std::move(key), std::move(val)));
	}
}

template<typename KeyType, typename ValueType>
inline void threadsafe_lut<KeyType, ValueType>::remove_mapping(KeyType key)
{
	std::unique_lock lock(bucket_mutexes[hash(key)]);
	Bucket& bucket = get_bucket(key);
	auto pos = find_in_bucket(bucket, key);
	if (pos == bucket.end())
	{
		return;
	}
	bucket.erase(pos);
}

template<typename KeyType, typename ValueType>
inline std::shared_ptr<ValueType> threadsafe_lut<KeyType, ValueType>::value_for(KeyType key)
{
	std::shared_lock lock(bucket_mutexes[hash(key)]);
	Bucket& bucket = get_bucket(key);
	auto pos = find_in_bucket(bucket, key);
	if (pos == bucket.end())
	{
		return nullptr;
	}
	return std::make_shared<ValueType>(pos->second);
}

template<typename KeyType, typename ValueType>
inline bool threadsafe_lut<KeyType, ValueType>::value_for(KeyType key, ValueType& out_val)
{
	std::shared_lock lock(bucket_mutexes[hash(key)]);
	Bucket& bucket = get_bucket(key);
	auto pos = find_in_bucket(bucket, key);
	if (pos == bucket.end())
	{
		return false;
	}
	out_val = pos->second;
	return true;
}

