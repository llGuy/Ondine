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

#define PANIC_AND_EXIT() exit(-1)
