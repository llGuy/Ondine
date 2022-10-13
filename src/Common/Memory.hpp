#pragma once

#include <new>
#include <utility>
#include <stdint.h>
#include <stddef.h>
#include "LinearAllocator.hpp"

namespace Ondine {

inline constexpr uint32_t kilobytes(uint32_t kb) {
    return(kb * 1024);
}

inline constexpr uint32_t megabytes(uint32_t mb) {
    return(kilobytes(mb * 1024));
}

/* Short hands for gLinearAllocator->* */
void *lnAlloc(size_t size);
void lnClear();

void freezeLinearAllocator();
void unfreezeLinearAllocator();

template <typename T, typename ...Args>
T *lnEmplaceAlloc(Args &&...args) {
  return Core::gLinearAllocator->emplaceAlloc<T>(std::forward<Args>(args)...);
}

template <typename T>
T *lnAllocv(size_t count) {
  return (T *)Core::gLinearAllocator->alloc(count * sizeof(T));
}

/* Free list allocator (TODO) for now just calls new */
template <typename T, typename ...Args>
T *flAlloc(Args &&...args) {
  return new T(std::forward<Args>(args)...);
}

template <typename T, typename ...Args>
T *flAllocv(size_t count, Args &&...args) {
  return new T[count] {std::forward<Args>(args)...};
}

template <typename T>
void flFree(T *ptr) {
  delete ptr;
}

template <typename T>
void flFreev(T *ptr) {
  delete[] ptr;
}

}
