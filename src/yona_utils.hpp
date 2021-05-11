#pragma once

#include <utility>
#include <stdint.h>
#include <stdlib.h>

#define STACK_ALLOC(type, n) (type *)alloca(sizeof(type) * (n))
#define BIT(n) (1 << n)

#ifndef YONA_PROJECT_ROOT
// This should never happen - just for intellisense (?)
#define YONA_PROJECT_ROOT ""
#endif

#define PANIC_AND_EXIT()                        \
  printf("\n***STOPPING SESSION***\n");         \
  exit(-1)

inline uint32_t popCount(
    uint32_t bits) {
#ifndef __GNUC__
    return __popcnt(bits);
#else
    return __builtin_popcount(bits);
#endif
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
