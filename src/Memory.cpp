#include <stdlib.h>
#include "Memory.hpp"

namespace Yona {

LinearAllocator::LinearAllocator(uint32_t maxSize)
  : mStart(nullptr),
    mCurrent(nullptr),
    mMaxSize(maxSize) {
  
}

LinearAllocator::~LinearAllocator() {
  free(mStart);
}

void LinearAllocator::init() {
  mStart = mCurrent = malloc(mMaxSize);
}

void *LinearAllocator::alloc(size_t size) {
  void *p = mCurrent;
  mCurrent = (void *)((uint8_t *)(mCurrent) + size);
  return p;
}

void LinearAllocator::clear() {
  mCurrent = mStart;
}

LinearAllocator *gLinearAllocator = nullptr;

void *lnAlloc(size_t size) {
  return gLinearAllocator->alloc(size);
}

void lnClear() {
  gLinearAllocator->clear();
}

}
