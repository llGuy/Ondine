#pragma once

#include <new>
#include <stdint.h>
#include <iterator>
#include "Buffer.hpp"

namespace Ondine {

constexpr int INVALID_NUMERIC_MAP_KEY = -1;

using NumericMapKey = int;

/* 
   Keeps a contiguous list of elements such that removing an element 
   Doesn't result in the altering of other elements' indices
   (Hence it's kind of like a pseudo-ordered list + hash map)
*/
template <typename T>
class NumericMap {
public:
  struct Header {
    uint64_t isUsed : 1;

    inline int next() {return (int)mNextNode - 1;}
    inline void setNext(int index) {mNextNode = (uint32_t)(index + 1);}
    inline int prev() {return (int)mPrevNode - 1;}
    inline void setPrev(int index) {mPrevNode = (uint32_t)(index + 1);}

  private:
    // Used only for occupied space
    uint64_t mPrevNode : 31;

    // Used for both free space and occupied space
    uint64_t mNextNode : 32;
  };

  struct Node {
    Header header;
    T value;
  };

  void init(uint32_t maxNodes) {
    mNodes.init(maxNodes);
    mFirstOccupied = INVALID_NUMERIC_MAP_KEY;
    mLastOccupied = INVALID_NUMERIC_MAP_KEY;
    mFirstFree = INVALID_NUMERIC_MAP_KEY;
    mUsedCapacity = 0;
  }

  NumericMapKey add(const T &element) {
    int key = INVALID_NUMERIC_MAP_KEY;

    if (mFirstFree == INVALID_NUMERIC_MAP_KEY) {
      // Need to extend the used capacity
      assert(mUsedCapacity < mNodes.capacity);

      key = mUsedCapacity++;
      Node *newNodeSpace = &mNodes[key];
      new(&newNodeSpace->value) T(element);
      newNodeSpace->header.isUsed = 1;
      newNodeSpace->header.setNext(INVALID_NUMERIC_MAP_KEY);

      setLastOccupied(key);
    }
    else {
      int freeNode = mFirstFree;
      Node *freeSpace = &mNodes[freeNode];
      mFirstFree = freeSpace->header.next();

      new(&freeSpace->value) T(element);
      freeSpace->header.isUsed = 1;
      freeSpace->header.setNext(INVALID_NUMERIC_MAP_KEY);

      setLastOccupied(freeNode);

      key = freeNode;
    }

    if (mFirstOccupied == INVALID_NUMERIC_MAP_KEY) {
      mFirstOccupied = key;
    }

    return key;
  }

  void remove(NumericMapKey key) {
    Node *node = getNode(key);

    if (key == mFirstOccupied) {
      mFirstOccupied = node->header.next();
    }

    if (key == mLastOccupied) {
      mLastOccupied = node->header.prev();
    }

    // Sort out occupied nodes linked list
    if (node->header.prev() != INVALID_NUMERIC_MAP_KEY) {
      Node *prev = getNode(node->header.prev());
      prev->header.setNext(node->header.next());
    }

    if (node->header.next() != INVALID_NUMERIC_MAP_KEY) {
      Node *next = getNode(node->header.next());
      next->header.setPrev(node->header.prev());
    }

    // Sort out removed nodes linked list
    if (node) {
      node->header.setNext(mFirstFree);
      node->header.isUsed = 0;

      mFirstFree = key;
    }
  }

  T &operator[](uint32_t index) {
    return mNodes[index].value;
  }

  const T &operator[](uint32_t index) const {
    return mNodes[index].value;
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

    iterator(NumericMap<T> *container, Node *element, NumericMapKey key)
      : mContainer(container), mNode(element), mKey(key) {
      
    }

    self_type operator++() {
      self_type current = *this;
      NumericMapKey nextKey = mNode->header.next();
      mNode = mContainer->getNode(nextKey);
      mKey = nextKey;
      return current;
    }

    self_type operator++(int) {
      NumericMapKey nextKey = mNode->header.next();
      mNode = mContainer->getNode(nextKey);
      mKey = nextKey;
      return *this;
    }

    reference operator*() {return mNode->value;}
    pointer operator->() {return &mNode->value;}
    bool operator==(const self_type &rhs) {return this->mKey == rhs.mKey;}
    bool operator!=(const self_type &rhs) {return this->mKey != rhs.mKey;}

  private:
    NumericMap<T> *mContainer;
    Node *mNode;
    NumericMapKey mKey;
  };

  class const_iterator {
  public:
    using value_type = T;
    using reference = T &;
    using self_type = const_iterator;
    using pointer = T *;
    using difference_type = int;
    using iterator_category = std::forward_iterator_tag;

    const_iterator(NumericMap<T> *container, Node *element, NumericMapKey key)
      : mContainer(container), mNode(element) {
      
    }

    self_type operator++() {
      self_type current = *this;
      NumericMapKey nextKey = mNode->header.next();
      mNode = mContainer->getNode(nextKey);
      mKey = nextKey;
      return current;
    }

    self_type operator++(int) {
      NumericMapKey nextKey = mNode->header.next();
      mNode = mContainer->getNode(nextKey);
      mKey = nextKey;
      return *this;
    }

    const reference operator*() {return mNode->value;}
    const pointer operator->() {return &mNode->value;}
    bool operator==(const self_type &rhs) {return this->mKey == rhs.mKey;}
    bool operator!=(const self_type &rhs) {return this->mKey != rhs.mKey;}

  private:
    NumericMap<T> *mContainer;
    Node *mNode;
    NumericMapKey mKey;
  };

  iterator begin() {
    return iterator(this, getNode(mFirstOccupied), mFirstOccupied);
  }

  iterator end() {
    return iterator(this, nullptr, INVALID_NUMERIC_MAP_KEY);
  }

  const_iterator begin() const {
    return iterator(this, getNode(mFirstOccupied), mFirstOccupied);
  }

  const_iterator end() const {
    return const_iterator(this, nullptr, INVALID_NUMERIC_MAP_KEY);
  }

private:
  void setLastOccupied(int key) {
    if (mLastOccupied == INVALID_NUMERIC_MAP_KEY) {
      mLastOccupied = key;

      getNode(key)->header.setPrev(INVALID_NUMERIC_MAP_KEY);
    }
    else {
      Node *last = &mNodes[mLastOccupied];
      last->header.setNext(key);
      getNode(key)->header.setPrev(mLastOccupied);
      mLastOccupied = key;
    }
  }

  Node *getNode(int key) {
    if (key == INVALID_NUMERIC_MAP_KEY) {
      return nullptr;
    }
    else {
      return &mNodes[key];
    }
  }

private:
  int mFirstFree;
  int mFirstOccupied;
  int mLastOccupied;
  uint32_t mUsedCapacity;
  Array<Node> mNodes;
  Node mNullNode;

  friend class iterator;
};

}
