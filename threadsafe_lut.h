#pragma once
#include <cassert>
#include <functional>
#include <type_traits>
#include <vector>
#include <utility>
#include <algorithm>
#include <shared_mutex>
#include <memory>

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

