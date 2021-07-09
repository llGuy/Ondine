#pragma once

#include <new>
#include "Log.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"
#include <unordered_map>

namespace Ondine {

// TODO: Implement custom map which better suits the max entry philosophy
template <typename Key, typename T>
using Map = std::unordered_map<Key, T>;

using FastMapHandle = uint32_t;

constexpr FastMapHandle FAST_TRACKER_HANDLE_INVALID = 0xFFFFFFFF;

/* 
   Optimal usage pattern:
   Creating a bunch of entries at the beginning of the program lifetime
   then during runtime simply accessing the entries with an ID handle
 */
template <typename Key, typename T, int MaxEntries>
class FastMapStd {
public:
  void init() {
    mEntries.init(MaxEntries);
  }

  void insert(const Key &key, const T &value) {
    FastMapHandle handle = mEntries.size;
    mMap.insert({key, handle});
    mEntries[mEntries.size++] = value;
  }

  template <typename ...Ts>
  T &emplace(const Key &key, Ts &&...constructor) {
    FastMapHandle handle = mEntries.size;
    mMap.insert({key, handle});
    new (&mEntries[mEntries.size]) T(std::forward<Ts>(constructor)...);
    return mEntries[mEntries.size++];
  }

  FastMapHandle getHandle(const Key &key) {
    return mMap.at(key);
  }

  T &getEntry(const Key &key) {
    return mEntries[getHandle(key)];
  }

  T &getEntry(FastMapHandle handle) {
    return mEntries[handle];
  }

  FastMapHandle getHandle(const Key &key) const {
    return mMap.at(key);
  }

  const T &getEntry(const Key &key) const {
    return mEntries[getHandle(key)];
  }

  const T &getEntry(FastMapHandle handle) const {
    return mEntries[handle];
  }

private:
  Array<T> mEntries;
  Map<Key, FastMapHandle> mMap;
};

// Simple map with predetermined size
template <
  typename T,
  uint32_t BucketCount,
  uint32_t BucketSize,
  uint32_t BucketSearchCount>
class FastMap {
public:
  enum { UNINITIALISED_HASH = 0xFFFFFFFF };
  enum { ITEM_POUR_LIMIT    = BucketSearchCount };

  struct Item {
    uint32_t hash = UNINITIALISED_HASH;
    T value = T();
  };

  struct Bucket {
    uint32_t bucket_usage_count = 0;
    Item items[BucketSize] = {};
  };

  void init() {
    for (uint32_t bucketIndex = 0; bucketIndex < BucketCount; ++bucketIndex) {
      for (uint32_t itemIndex = 0; itemIndex < BucketSize; ++itemIndex) {
        mBuckets[bucketIndex].items[itemIndex].hash = UNINITIALISED_HASH;
      }
    }
  }

  void clear() {
    for (uint32_t i = 0; i < BucketCount; ++i) {
      mBuckets[i].bucket_usage_count = 0;

      for (uint32_t item = 0; item < BucketSize; ++item) {
        mBuckets[i].items[item].hash = UNINITIALISED_HASH;
      }
    }
  }
    
  void cleanUp() {
    for (uint32_t i = 0; i < BucketCount; ++i) {
      mBuckets[i].bucket_usage_count = 0;
    }
  }

  void insert(uint32_t hash, T value) {
    Bucket *bucket = &mBuckets[hash % BucketCount];

    for (uint32_t bucketItem = 0; bucketItem < BucketSize; ++bucketItem) {
      Item *item = &bucket->items[bucketItem];
      if (item->hash == UNINITIALISED_HASH) {
        /* found a slot for the object */
        item->hash = hash;
        item->value = value;
        return;
      }
    }
        
    LOG_ERROR("Fatal error in hash table insert()\n");
    PANIC_AND_EXIT();
  }
    
  void remove(uint32_t hash) {
    Bucket *bucket = &mBuckets[hash % BucketCount];

    for (uint32_t bucketItem = 0; bucketItem < BucketSize; ++bucketItem) {

      Item *item = &bucket->items[bucketItem];

      if (item->hash == hash && item->hash != UNINITIALISED_HASH) {
        item->hash = UNINITIALISED_HASH;
        item->value = T();
        return;
      }
    }

    LOG_ERROR("Error in hash table remove()\n");
    PANIC_AND_EXIT();
  }

  const T *get(uint32_t hash) const {
    static int32_t invalid = -1;
    const Bucket *bucket = &mBuckets[hash % BucketCount];
    uint32_t bucketItem = 0;
    uint32_t filledItems = 0;

    for (; bucketItem < BucketSize; ++bucketItem) {
      const Item *item = &bucket->items[bucketItem];
      if (item->hash != UNINITIALISED_HASH) {
        ++filledItems;
        if (hash == item->hash) {
          return &item->value;
        }
      }
    }

    if (filledItems == BucketSize) {
      LOG_ERROR("Error in hash table get()\n");
    }

    PANIC_AND_EXIT();
    return nullptr;
  }
    
  T *get(uint32_t hash) {
    static int32_t invalid = -1;
    Bucket *bucket = &mBuckets[hash % BucketCount];
    uint32_t bucketItem = 0;
    uint32_t filledItems = 0;

    for (; bucketItem < BucketSize; ++bucketItem) {
      Item *item = &bucket->items[bucketItem];
      if (item->hash != UNINITIALISED_HASH) {
        ++filledItems;
        if (hash == item->hash) {
          return &item->value;
        }
      }
    }

    if (filledItems == BucketSize) {
      LOG_ERROR("Error in hash table get()\n");
    }

    PANIC_AND_EXIT();
    return nullptr;
  }

private:
  Bucket mBuckets[BucketCount] = {};
};

}
