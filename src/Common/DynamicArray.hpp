#pragma once

#include <stdint.h>
#include <iterator>
#include "Memory.hpp"

namespace Ondine {

/* 
   Container which allows very fast removal of elements 
   Compromise: there's a max amount of elements
   To add: dynamic growth of the array
*/
template <typename T>
class DynamicArray {
public:
  DynamicArray() 
    : mSize(0),
      mMaxElements(0),
      mRemovedCount(0),
      mData(nullptr),
      mRemoved(nullptr) {

  }

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

    flFreev(mData);
    flFreev(mRemoved);

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

public:
  class iterator {
  public:
    using value_type = T;
    using reference = T &;
    using self_type = iterator;
    using pointer = T *;
    using difference_type = int;
    using iterator_category = std::forward_iterator_tag;

    iterator(DynamicArray<T> *container, uint32_t index)
      : mContainer(container), mIndex(index), mRemovedIndex(0) {
      
    }

    self_type operator++() {
      self_type current = *this;

      if ((mContainer->mRemovedCount - mRemovedIndex) && 
        mContainer->mRemoved[mRemovedIndex] == mIndex) {
        do {
          ++mRemovedIndex;
          ++mIndex;
        } while((mContainer->mRemovedCount - mRemovedIndex) && 
            mContainer->mRemoved[mRemovedIndex] == mIndex);
      }
      else {
        ++mIndex;
      }

      return current;
    }

    self_type operator++(int) {
      if ((mContainer->mRemovedCount - mRemovedIndex) && 
        mContainer->mRemoved[mRemovedIndex] == mIndex) {
        do {
          ++mRemovedIndex;
          ++mIndex;
        } while((mContainer->mRemovedCount - mRemovedIndex) && 
            mContainer->mRemoved[mRemovedIndex] == mIndex);
      }
      else {
        ++mIndex;
      }

      return *this;
    }

    reference operator*() {return mContainer->mData[mIndex];}
    pointer operator->() {return &mContainer->mData[mIndex];}
    bool operator==(const self_type &rhs) {return this->mIndex == rhs.mIndex;}
    bool operator!=(const self_type &rhs) {return this->mIndex != rhs.mIndex;}

  private:
    DynamicArray<T> *mContainer;
    uint32_t mIndex;
    uint32_t mRemovedIndex;
  };

  class const_iterator {
  public:
    using value_type = T;
    using reference = T &;
    using self_type = const_iterator;
    using pointer = T *;
    using difference_type = int;
    using iterator_category = std::forward_iterator_tag;

    const_iterator(const DynamicArray<T> *container, uint32_t index)
      : mContainer(container), mIndex(index), mRemovedIndex(0) {
      
    }

    self_type operator++() {
      self_type current = *this;

      if ((mContainer->mRemovedCount - mRemovedIndex) && 
        mContainer->mRemoved[mRemovedIndex] == mIndex) {
        do {
          ++mRemovedIndex;
          ++mIndex;
        } while((mContainer->mRemovedCount - mRemovedIndex) && 
            mContainer->mRemoved[mRemovedIndex] == mIndex);
      }
      else {
        ++mIndex;
      }

      return current;
    }

    self_type operator++(int) {
      if ((mContainer->mRemovedCount - mRemovedIndex) && 
        mContainer->mRemoved[mRemovedIndex] == mIndex) {
        do {
          ++mRemovedIndex;
          ++mIndex;
        } while((mContainer->mRemovedCount - mRemovedIndex) && 
            mContainer->mRemoved[mRemovedIndex] == mIndex);
      }
      else {
        ++mIndex;
      }

      return *this;
    }

    reference operator*() {return mContainer->mData[mIndex];}
    pointer operator->() {return &mContainer->mData[mIndex];}
    bool operator==(const self_type &rhs) {return this->mIndex == rhs.mIndex;}
    bool operator!=(const self_type &rhs) {return this->mIndex != rhs.mIndex;}
  private:
    DynamicArray<T> *mContainer;
    uint32_t mIndex;
    uint32_t mRemovedIndex;
  };

  iterator begin() {
    return iterator(this, 0);
  }

  iterator end() {
    return iterator(this, mSize);
  }

  const_iterator begin() const {
    return iterator(this, 0);
  }

  const_iterator at(const const_iterator &other) const {
    const_iterator res = {};
    res.mContainer = other.mContainer;
    res.mIndex = other.mIndex;
    res.mRemovedIndex = other.mRemovedIndex;

    return res;
  }

  const_iterator end() const {
    return const_iterator(this, mSize);
  }

private:
  T *mData;
  uint32_t mSize;
  const uint32_t mMaxElements;

  uint32_t mRemovedCount;
  uint32_t *mRemoved;
};

}
