#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

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

inline bool isSameDirection(const glm::vec3 &a, const glm::vec3 &b) {
  return glm::dot(a, b) > 0.0f;
}

inline glm::vec3 getBarycentricCoordinates(
  const glm::vec3 &p,
  const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
  glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
  float d00 = glm::dot(v0, v0);
  float d01 = glm::dot(v0, v1);
  float d11 = glm::dot(v1, v1);
  float d20 = glm::dot(v2, v0);
  float d21 = glm::dot(v2, v1);
  float denom = d00 * d11 - d01 * d01;
  float v = (d11 * d20 - d01 * d21) / denom;
  float w = (d00 * d21 - d01 * d20) / denom;
  float u = 1.0f - v - w;
  return glm::vec3(u, v, w);
}

static constexpr uint32_t kMaxUint32 = 0xFFFFFFFF;

struct Plane {
  glm::vec3 point;
  // Has to be normalized
  glm::vec3 normal;
};

// Returns the signed distance
inline float getDistanceFromPlane(const Plane &plane, const glm::vec3 &a) {
  float adotn = glm::dot(a, plane.normal);
  float pdotn = glm::dot(plane.point, plane.normal);
  return (adotn - pdotn);
}

}