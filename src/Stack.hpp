#pragma once

#include <new>
#include <stdint.h>
#include <assert.h>
#include <iterator>
#include "Buffer.hpp"

namespace Ondine {

template <typename T>
class Stack {
public:
  void init(uint32_t maxItems) {
    mItems.init(maxItems);
    mCurrent = 0;
    mMax = maxItems;
  }

  uint32_t push(const T &element) {
    assert(mCurrent < mMax);

    new(&mItems[mCurrent]) T(element);
    return mCurrent++;
  }

  void clear() {
    mCurrent = 0;
  }

  T pop() {
    assert(mCurrent > 0);
    return mItems[mCurrent--];
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

    iterator(Stack<T> *container, uint32_t index)
      : mContainer(container), mIndex(index) {
      
    }

    self_type operator++() {
      self_type current = *this;
      ++mIndex;
      return current;
    }

    self_type operator++(int) {
      ++mIndex;
      return *this;
    }

    reference operator*() {return mContainer->mItems[mIndex];}
    pointer operator->() {return &mContainer->mItems[mIndex];}
    bool operator==(const self_type &rhs) {return this->mIndex == rhs.mIndex;}
    bool operator!=(const self_type &rhs) {return this->mIndex != rhs.mIndex;}

  private:
    Stack<T> *mContainer;
    uint32_t mIndex;
  };

  class const_iterator {
  public:
    using value_type = T;
    using reference = T &;
    using self_type = const_iterator;
    using pointer = T *;
    using difference_type = int;
    using iterator_category = std::forward_iterator_tag;

    const_iterator(const Stack<T> *container, uint32_t index)
      : mContainer(container), mIndex(index) {
      
    }

    self_type operator++() {
      self_type current = *this;
      ++mIndex;
      return current;
    }

    self_type operator++(int) {
      ++mIndex;
      return *this;
    }

    reference operator*() {return mContainer->mItems[mIndex];}
    pointer operator->() {return &mContainer->mItems[mIndex];}
    bool operator==(const self_type &rhs) {return this->mIndex == rhs.mIndex;}
    bool operator!=(const self_type &rhs) {return this->mIndex != rhs.mIndex;}

  private:
    Stack<T> *mContainer;
    uint32_t mIndex;
  };

  iterator begin() {
    return iterator(this, 0);
  }

  iterator end() {
    return iterator(this, mCurrent);
  }

  const_iterator begin() const {
    return iterator(this, 0);
  }

  const_iterator end() const {
    return const_iterator(this, mCurrent);
  }

private:
  Array<T> mItems;
  uint32_t mCurrent;
  uint32_t mMax;
};

}
