#pragma once

#include <new>
#include <utility>
#include <stdint.h>
#include <stddef.h>

namespace Yona {

inline uint32_t kilobytes(uint32_t kb) {
    return(kb * 1024);
}

inline uint32_t megabytes(uint32_t mb) {
    return(kilobytes(mb * 1024));
}

/* There will be a global instance */
class LinearAllocator {
public:
  LinearAllocator(uint32_t maxSize);
  ~LinearAllocator();

  /* Gives possibility to control when the bulk gets allocated */
  void init();

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

/* Short hands for gLinearAllocator->* */
void *lnAlloc(size_t size);
void lnClear();

template <typename T, typename ...Args>
T *lnEmplaceAlloc(Args &&...args) {
  return gLinearAllocator->emplaceAlloc<T>(std::forward<Args>(args)...);
}

/* Free list allocator (TODO) */
template <typename T, typename ...Args>
T *flAlloc(Args &&...args) {
  return new T(std::forward<Args>(args)...);
}

template <typename T>
void flFree(T *ptr) {
  delete ptr;
}

}
