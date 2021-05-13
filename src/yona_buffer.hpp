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

enum class AllocationType {
  Linear,
  Freelist
};

template <typename T, AllocationType A = AllocationType::Freelist>
struct Array {
  T *data;
  size_t size;
  size_t capacity;

  Array() = default;

  Array(size_t capacity)
    : capacity(capacity), size(0) {
    if constexpr (A == AllocationType::Linear) {
      data = lnAllocv<T>(capacity);
    }
    else {
      data = flAllocv<T>(capacity);
    }
  }

  Array(T *ptr, size_t count)
    : data(ptr), size(count), capacity(count) {
    
  }

  void init(size_t cap) {
    capacity = cap;
    size = 0;

    if constexpr (A == AllocationType::Linear) {
      data = lnAllocv<T>(cap);
    }
    else {
      data = flAllocv<T>(cap);
    }
  }

  void init(T *ptr, size_t count) {
    capacity = count;
    size = count;
    data = ptr;
  }

  void free() {
    if constexpr (A == AllocationType::Freelist) {
      flFreev(data);
    }
  }

  inline T &operator[](size_t index) {
    return data[index];
  }

  inline const T &operator[](size_t index) const {
    return data[index];
  }
};

template <typename T, AllocationType A, typename ...Args>
Array<T, A> makeArray(Args &&...args) {
  Array<T, A> arr (sizeof...(Args));

  auto addElement = [] (Array<T, A> &array, T &&element) {
    array[array.size++] = std::forward<T>(element);
  };

  /* Dirty trick */
  char dummy[] = { (char)0, (addElement(arr, std::forward<Args>(args)), (char)0)... };

  return arr;
}

}
