#include <assert.h>
#include <stdlib.h>
#include "LinearAllocator.hpp"

namespace Ondine::Core {

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
  assert(mCurrent < (uint8_t *)mStart + mMaxSize);
  mCurrent = (void *)((uint8_t *)(mCurrent) + size);
  return p;
}

void LinearAllocator::clear() {
  mCurrent = mStart;
}

LinearAllocator *gLinearAllocator = nullptr;

}
