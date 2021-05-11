#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "yona_memory.hpp"

namespace Yona {

/* Fixed size */
struct Buffer {
  uint8_t *data;
  size_t size;
};

template <typename T>
struct Array {
  T *data;
  size_t count;

  Array() = default;

  Array(size_t count)
    : count(count) {
    data = flAlloc<T>(count);
  }

  void init(size_t size) {
    count = size;
    data = flAlloc<T>(size);
  }

  inline T &operator[](size_t index) {
    return data[index];
  }
};

}
