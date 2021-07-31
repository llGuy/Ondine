#pragma once

#include <stdint.h>

namespace Ondine {

/* Only supports allocation of a fixed size */
class ArenaAllocator {
public:
  ArenaAllocator() = default;
  ArenaAllocator(uint32_t maxSize, uint32_t allocSize);
  ~ArenaAllocator();

  void init(uint32_t maxSize, uint32_t allocSize);
  void *alloc();
  void free(void *ptr);

private:
  struct ArenaHeader {
    ArenaHeader *next;
  };

  void *mStart;
  ArenaHeader *mHead;
  uint32_t mMaxSize;
  uint32_t mAllocSize;
};

}
