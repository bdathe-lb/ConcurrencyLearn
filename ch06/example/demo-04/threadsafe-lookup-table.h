#ifndef DEMO_04_THREADSAFE_LOOKUP_TABLE_H
#define DEMO_04_THREADSAFE_LOOKUP_TABLE_H

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <memory>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <functional>
#include <shared_mutex>

namespace demo04 {

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadSafeLookupTable {
private:
  class Bucket {
    friend class ThreadSafeLookupTable;
  private:
    using Entry = std::pair<Key, Value>;
    using EntryList = std::list<Entry>;
    using EntryIterator = typename EntryList::iterator;
    using EntryConstIterator = typename EntryList::const_iterator;

    EntryList entries_;
    mutable std::shared_mutex mutex_;

  private:
    EntryIterator FindEntryFor(const Key& key);
    EntryConstIterator FindEntryFor(const Key& key) const;

  public:
    Value ValueFor(const Key& key, const Value& default_value) const;
    void AddOrUpdateMapping(const Key& key, const Value& value);
    void RemoveMapping(const Key& key);
  }; // class Bucket

  std::vector<std::unique_ptr<Bucket>> buckets_;
  Hash hasher_;

private:
  Bucket& GetBucket(const Key& key) const;

public:
  using Keytype = Key;
  using MappedType = Value;
  using HashType = Hash;

  explicit ThreadSafeLookupTable(std::size_t num_buckets = 19, const Hash& hasher = Hash());

  ThreadSafeLookupTable(const ThreadSafeLookupTable&) = delete;
  ThreadSafeLookupTable& operator=(const ThreadSafeLookupTable&) = delete;

  Value ValueFor(const Key& key, const Value& default_value = Value()) const;
  void AddOrUpdateMapping(const Key& key, const Value& value);
  void RemoveMapping(const Key& key);
  std::map<Key, Value> GetMap() const;
}; // class ThreadSafeLookupTable

template<typename Key, typename Value, typename Hash>
typename ThreadSafeLookupTable<Key, Value, Hash>::Bucket::EntryIterator
ThreadSafeLookupTable<Key, Value, Hash>::Bucket::FindEntryFor(const Key& key) {
  return std::find_if(entries_.begin(), entries_.end(),
                      [&](const Entry& item) { return item.first == key; });
}

template<typename Key, typename Value, typename Hash>
typename ThreadSafeLookupTable<Key, Value, Hash>::Bucket::EntryConstIterator
ThreadSafeLookupTable<Key, Value, Hash>::Bucket::FindEntryFor(const Key& key) const {
  return std::find_if(entries_.begin(), entries_.end(),
                      [&](const Entry& item) { return item.first == key; });
}

template<typename Key, typename Value, typename Hash>
Value
ThreadSafeLookupTable<Key, Value, Hash>::Bucket::ValueFor(const Key& key, const Value& default_value) const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  const EntryConstIterator found_entry = FindEntryFor(key);
  return found_entry == entries_.end() ? default_value : found_entry->second;
}

template<typename Key, typename Value, typename Hash>
void
ThreadSafeLookupTable<Key, Value, Hash>::Bucket::AddOrUpdateMapping(const Key& key, const Value& value) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  const EntryIterator found_entry = FindEntryFor(key);
  if (found_entry == entries_.end()) {
    entries_.emplace_back(key, value);
  } else {
    found_entry->second = value;
  }
}

template<typename Key, typename Value, typename Hash>
void
ThreadSafeLookupTable<Key, Value, Hash>::Bucket::RemoveMapping(const Key& key) {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  const EntryIterator found_entry = FindEntryFor(key);
  if (found_entry != entries_.end()) {
    entries_.erase(found_entry);
  }
}

template<typename Key, typename Value, typename Hash>
typename ThreadSafeLookupTable<Key, Value, Hash>::Bucket&
ThreadSafeLookupTable<Key, Value, Hash>::GetBucket(const Key& key) const {
  const std::size_t bucket_index = hasher_(key) % buckets_.size();
  return *buckets_[bucket_index];
}

template<typename Key, typename Value, typename Hash>
ThreadSafeLookupTable<Key, Value, Hash>::ThreadSafeLookupTable(std::size_t num_buckets, const Hash& hasher)
  : buckets_(num_buckets)
  , hasher_(hasher) {
  for (auto& bucket : buckets_) {
    bucket = std::make_unique<Bucket>();
  }
}

template<typename Key, typename Value, typename Hash>
Value
ThreadSafeLookupTable<Key, Value, Hash>::ValueFor(const Key& key, const Value& default_value) const {
  return GetBucket(key).ValueFor(key, default_value);
}

template<typename Key, typename Value, typename Hash>
void
ThreadSafeLookupTable<Key, Value, Hash>::AddOrUpdateMapping(const Key& key, const Value& value) {
  GetBucket(key).AddOrUpdateMapping(key, value);
}

template<typename Key, typename Value, typename Hash>
void
ThreadSafeLookupTable<Key, Value, Hash>::RemoveMapping(const Key& key) {
  GetBucket(key).RemoveMapping(key);
}

template<typename Key, typename Value, typename Hash>
std::map<Key, Value>
ThreadSafeLookupTable<Key, Value, Hash>::GetMap() const {
  std::vector<std::shared_lock<std::shared_mutex>> locks;
  locks.reserve(buckets_.size());
  for (const auto& bucket : buckets_) {
    locks.emplace_back(bucket->mutex_);
  }

  std::map<Key, Value> res;
  for (const auto& bucket : buckets_) {
    for (const auto& entry : bucket->entries_) {
      res.insert(entry);
    }
  }

  return res;
}

} // namespace demo04

#endif  // !DEMO_04_THREADSAFE_LOOKUP_TABLE_H
