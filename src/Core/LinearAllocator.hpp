#pragma once

#include <stdint.h>
#include <utility>

namespace Ondine::Core {

/* There will be a global instance */
class LinearAllocator {
public:
  LinearAllocator() = default;
  LinearAllocator(uint32_t maxSize);
  ~LinearAllocator();

  /* Gives possibility to control when the bulk gets allocated */
  void init();
  void init(uint32_t maxSize);

  void *alloc(size_t size);
  void clear();

  template <typename T, typename ...Args>
  T *emplaceAlloc(Args &&...args) {
    T *ptr = (T *)alloc(sizeof(T));
    new(ptr) T{std::forward<Args>(args)...};
    return ptr;
  }
private:
  void *mStart;
  void *mCurrent;
  uint32_t mMaxSize;
};

extern LinearAllocator *gLinearAllocator;

}
