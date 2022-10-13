#pragma once

#include <glm/glm.hpp>

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

static constexpr glm::vec3 kGlobalUp = glm::vec3(0.0f, 1.0f, 0.0f);

struct AABB {
  glm::vec3 pMin;
  glm::vec3 pMax;

  inline bool overlaps(const AABB &o) const {
    auto [a_min, a_max] = *this;
    auto [b_min, b_max] = o;

    return a_min.x < b_max.x && b_min.x < a_max.x &&
      a_min.y < b_max.y && b_min.y < a_max.y &&
      a_min.z < b_max.z && b_min.z < a_max.z;
  }

  inline void expand(const glm::vec3 &p) {
    if (p.x < pMin.x) {
      pMin.x = p.x;
    } else if (p.x > pMax.x) {
      pMax.x = p.x;
    }

    if (p.y < pMin.y) {
      pMin.y = p.y;
    } else if (p.y > pMax.y) {
      pMax.y = p.y;
    }

    if (p.z < pMin.z) {
      pMin.z = p.z;
    } else if (p.z > pMax.z) {
      pMax.z = p.z;
    }
  }

  static inline AABB invalid() {
    return AABB {
      .pMin = glm::vec3 {FLT_MAX, FLT_MAX, FLT_MAX},
      .pMax = glm::vec3 {FLT_MIN, FLT_MIN, FLT_MIN},
    };
  }

  static inline AABB point(const glm::vec3 &p) {
    return AABB {
      .pMin = p,
      .pMax = p,
    };
  }

  static inline AABB unitCube() {
    return AABB {
      .pMin = glm::vec3(-1.0f),
      .pMax = glm::vec3(1.0f)
    };
  }
};

inline AABB operator*(const glm::mat4 &transform, const AABB &box) {
  AABB res = box;
  res.pMin = glm::vec3(transform * glm::vec4(box.pMin, 1.0f));
  res.pMax = glm::vec3(transform * glm::vec4(box.pMax, 1.0f));
  return res;
}

}
