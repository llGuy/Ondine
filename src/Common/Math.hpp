#pragma once

namespace Ondine {

inline float lerp(float a, float b, float x) {
  if (b - a == 0.0f) {
    return 0.0f;
  }
  return (x - a) / (b - a);
}

template <typename T>
inline T interpolate(const T &a, const T &b, float progress) {
  if (b - a == (T)0) {
    return (T)0;
  }
    return a + progress * (b - a);
}

}
