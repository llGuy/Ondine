#include <assert.h>
#include <stdlib.h>
#include "Utils.hpp"
#include "ArenaAllocator.hpp"

namespace Ondine {

ArenaAllocator::ArenaAllocator(uint32_t maxSize, uint32_t allocSize)
  : mMaxSize(maxSize), mAllocSize(allocSize) {
  mStart = malloc(mMaxSize);
  mEnd = (void *)((uint8_t *)mStart + mMaxSize);
  zeroMemory(mStart, maxSize);
  mHead = (ArenaHeader *)mStart;
  mHead->next = nullptr;
}

ArenaAllocator::~ArenaAllocator() {
  free(mStart);
}

void ArenaAllocator::init(uint32_t maxSize, uint32_t allocSize) {
  mMaxSize = maxSize;
  mAllocSize = allocSize;
  mStart = malloc(mMaxSize);
  mEnd = (void *)((uint8_t *)mStart + mMaxSize);
  zeroMemory(mStart, maxSize);
  mHead = (ArenaHeader *)mStart;
  mHead->next = nullptr;
}

void *ArenaAllocator::alloc() {
  // Go to head first
  ArenaHeader *header = mHead;
  void *p = (void *)header;

  if (header->next) {
    mHead = header->next;
  }
  else {
    uint8_t *current = (uint8_t *)header;
    assert(current < (uint8_t *)mStart + mMaxSize);
    current += mAllocSize;
    mHead = (ArenaHeader *)current;
  }

  return p;
}

void ArenaAllocator::free(void *ptr) {
  ArenaHeader *newHeader = (ArenaHeader *)ptr;

  if (ptr < mHead) {
    newHeader->next = mHead;
    mHead = newHeader;
  }
  else {
    ArenaHeader *current = mHead;

    for (; current->next; current = current->next) {
      if (ptr < current->next) {
        newHeader->next = current->next;
        current->next = newHeader;
      }
    }

    newHeader->next = nullptr;
    current->next = newHeader;
  }
}

void ArenaAllocator::clear() {
  zeroMemory(mStart, mMaxSize);
  mHead = (ArenaHeader *)mStart;
  mHead->next = nullptr;
}

}
