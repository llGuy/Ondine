#pragma once

#include <utility>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define STACK_ALLOC(type, n) (type *)alloca(sizeof(type) * (n))
#define BIT(n) (1 << n)

#define DEFINE_BIT_OPS_FOR_ENUM_CLASS(enumClass, bitsType, baseType)    \
  inline bitsType operator|(enumClass a, enumClass b) {                 \
    return (bitsType)((baseType)a | (baseType)b);                       \
  }                                                                     \
  inline bitsType operator|(bitsType a, enumClass b) {                  \
    return (bitsType)((baseType)a | (baseType)b);                       \
  }                                                                     \
  inline bool operator&(bitsType a, enumClass b) {                      \
    return (bool)((baseType)a & (baseType)b);                           \
  }

#ifndef YONA_PROJECT_ROOT
// This should never happen - just for intellisense (?)
#define YONA_PROJECT_ROOT ""
#endif

#define PANIC_AND_EXIT()                        \
  printf("\n***STOPPING SESSION***\n");         \
  exit(-1)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

namespace Ondine {

struct Resolution {
  uint32_t width;
  uint32_t height;
};

inline uint32_t popCount(
    uint32_t bits) {
#ifndef __GNUC__
    return __popcnt(bits);
#else
    return __builtin_popcount(bits);
#endif
}

inline void zeroMemory(void *ptr, uint32_t size) {
  memset(ptr, 0, size);
}

}
