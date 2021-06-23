#pragma once

#include <stdint.h>
#include "Memory.hpp"

namespace Ondine {

/* 
   Container which allows very fast removal of elements 
   Compromise: there's a max amount of elements
*/
template <typename T>
class DynamicArray {
public:
  DynamicArray(uint32_t max)
    : mSize(0),
      mMaxElements(max),
      mRemovedCount(0) {
    mData = flAllocv<T>(mMaxElements);
    mRemoved = flAllocv<uint32_t>(mMaxElements);
    memset(mData, 0, sizeof(T) * mMaxElements);
  }

  ~DynamicArray() {
    clear();

    flFree(mData);
    flFree(mRemoved);

    mData = nullptr;
    mRemoved = nullptr;
  }

  uint32_t add() {
    if (mRemovedCount) {
      return mRemoved[mRemovedCount-- - 1];
    }
    else {
      return mSize++;
    }
  }

  T &operator[](uint32_t index) {
    return mData[index];
  }

  const T &operator[](uint32_t index) const {
    return mData[index];
  }

  void remove(uint32_t index) {
    mData[index] = T();
    mRemoved[mRemovedCount++] = index;
  }

  void clear() {
    mSize = 0;
    mRemovedCount = 0;
  }

  uint32_t size() const {
    return mSize;
  }

private:
  T *mData;
  uint32_t mSize;
  const uint32_t mMaxElements;

  uint32_t mRemovedCount;
  uint32_t *mRemoved;
};

}
