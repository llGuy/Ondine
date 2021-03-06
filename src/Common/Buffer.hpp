#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "Memory.hpp"

namespace Ondine {

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
  size_t capacity;
  size_t size;

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

  void zero() {
    memset(data, 0, sizeof(T) * capacity);
  }

  uint32_t memCapacity() {
    return sizeof(T) * capacity;
  }

  uint32_t memSize() {
    return sizeof(T) * size;
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

  auto addElement = [] (Array<T, A> &array, const T &element) {
    array[array.size++] = element;
  };

  /* Dirty trick */
  char dummy[] = {
    (char)0,
    (addElement(arr, std::forward<Args>(args)), (char)0)...
  };

  /* In case args is empty */
  (void)dummy;
  (void)addElement;

  return arr;
}

template <typename T, AllocationType A, typename Pred, typename ...Args>
Array<T, A> makeArrayPred(Pred pred, Args &&...args) {
  Array<T, A> arr (sizeof...(Args));

  auto addElement = [&pred] (Array<T, A> &array, auto &&element) {
    array[array.size++] = pred(std::forward<decltype(element)>(element));
  };

  /* Dirty trick */
  char dummy[] = {
    (char)0,
    (addElement(arr, std::forward<decltype(args)>(args)), (char)0)...
  };

  /* In case args is empty */
  (void)dummy;
  (void)addElement;

  return arr;
}

}
