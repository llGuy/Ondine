#pragma once

#include "Math.hpp"
#include "Model.hpp"

namespace Ondine::Game {

struct Entity;

}

namespace Ondine::Game::Physics {

// In order to allow for physics collisions, we must create a collision mesh
// for each entity each frame. Then, just call gjkOverlap to see whether there was a collision!

struct Simplex {
  // Don't need more than 4 vertices because we are in 3D
  glm::vec3 vertices[4];
  uint32_t size;

  Simplex() : size(0) {}

  // Adds points to front of array with truncation
  void add(const glm::vec3 &point) {
    size = glm::min(4u, size + 1);
    glm::vec3 newItems[] = { point, vertices[0], vertices[1], vertices[2] };
    memcpy(vertices, newItems, sizeof(glm::vec3) * 4);
  }

  glm::vec3 &operator[](uint32_t i) {
    return vertices[i];
  }
};

template <typename ...T>
Simplex createSimplex(T &&...ts) {
  Simplex simplex;
  glm::vec3 items[] = { ts... };
  memcpy(simplex.vertices, items, sizeof(glm::vec3) * (sizeof...(T)));
  simplex.size = sizeof...(T);
  return simplex;
}

bool nextSimplex(Simplex &simplex, const glm::vec3 &d);

// Contains transformed vertices for given geometry
struct CollisionMesh {
  uint32_t vertexCount;
  glm::vec3 *vertices;
};

// Allocates in the linear allocator so cheap allocation
CollisionMesh createCollisionMesh(const Graphics::Geometry &geometry, const Entity &entity);

// Finds the furthest point from the origin
glm::vec3 findFurthestVertex(const CollisionMesh &geometry, const glm::vec3 &d);

// Finds the furthest point from the origin in the Minkowski difference (support point)
glm::vec3 findSupportPointMinkowski(
  const CollisionMesh &a,
  const CollisionMesh &b,
  const glm::vec3 &d);

// Uses GJK algorithm to detect if two meshes overlap
struct GJK {
  bool bOverlapped;
  Simplex simplex;
};

GJK gjkOverlap(const CollisionMesh &a, const CollisionMesh &b);

struct Collision {
  bool bDetectedCollision;
  glm::vec3 normal;
  float depth;
};

Collision detectCollision(const CollisionMesh &a, const CollisionMesh &b);

}
