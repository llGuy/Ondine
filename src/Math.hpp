#pragma once

namespace Ondine {

inline float lerp(float a, float b, float x) {
  return (x - a) / (b - a);
}

template <typename T>
inline T interpolate(const T &a, const T &b, float progress) {
    return a + progress * (b - a);
}

}
