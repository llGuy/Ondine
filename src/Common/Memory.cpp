#include <cassert>
#include <stdlib.h>
#include "Memory.hpp"

namespace Ondine {

static bool bFreezeLinearAllocator = false;

void *lnAlloc(size_t size) {
  // Make sure that linear allocator isn't frozen
  assert(!bFreezeLinearAllocator);
  return Core::gLinearAllocator->alloc(size);
}

void lnClear() {
  assert(!bFreezeLinearAllocator);
  Core::gLinearAllocator->clear();
}

void freezeLinearAllocator() {
  bFreezeLinearAllocator = true;
}

void unfreezeLinearAllocator() {
  bFreezeLinearAllocator = false;
}

}
