#pragma once

#include <new>
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

}
