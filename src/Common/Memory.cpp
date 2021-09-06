#include <stdlib.h>
#include "Memory.hpp"

namespace Ondine {

void *lnAlloc(size_t size) {
  return Core::gLinearAllocator->alloc(size);
}

void lnClear() {
  Core::gLinearAllocator->clear();
}

}
