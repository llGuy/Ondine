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
  size_t size;
  size_t capacity;

  Array() = default;

  Array(size_t capacity)
    : capacity(capacity), size(0) {
    data = flAlloc<T>(capacity);
  }

  Array(T *ptr, size_t count)
    : data(ptr), size(count), capacity(count) {
    
  }

  void init(size_t cap) {
    capacity = cap;
    size = 0;
    data = flAllocv<T>(size);
  }

  void init(T *ptr, size_t count) {
    capacity = count;
    size = count;
    data = ptr;
  }

  void free() {
    flFreev(data);
  }

  inline T &operator[](size_t index) {
    return data[index];
  }

  inline const T &operator[](size_t index) const {
    return data[index];
  }
};

}
