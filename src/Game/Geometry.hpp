#pragma once

#include "Math.hpp"
#include "Memory.hpp"

namespace Ondine::Game::Physics {

struct FastPolygonList {
  uint32_t maxIndices;
  uint32_t *buffer;
  uint32_t size;
  uint32_t edgeCount;
  uint32_t polygonCount;

  FastPolygonList &operator=(const FastPolygonList &other) {
    allocate(other.maxIndices);
    this->size = other.size;
    this->edgeCount = other.edgeCount;
    this->polygonCount = other.polygonCount;

    memcpy(buffer, other.buffer, sizeof(uint32_t) * maxIndices);

    return *this;
  }

  void constructCube() {
    allocate(5 * 6);

    // anti clockwise
    addPolygon(4, 0, 1, 2, 3); // -Z
    addPolygon(4, 7, 6, 5, 4); // +Z
    addPolygon(4, 3, 2, 6, 7); // +Y
    addPolygon(4, 4, 5, 1, 0); // -Y
    addPolygon(4, 5, 6, 2, 1); // +X
    addPolygon(4, 0, 3, 7, 4); // -X
  }

  void allocate(uint32_t maxIdx) {
    maxIndices = maxIdx;
    buffer = flAllocv<uint32_t>(maxIndices);
    size = 0;
  }

  void free() {
    flFreev(buffer);
  }

  // Creation
  template <typename ...T>
  void addPolygon(uint32_t count, T &&...vertexIndices) {
    uint32_t indexCount = 1 + sizeof...(vertexIndices);
    uint32_t indices[] = {count, (uint32_t)vertexIndices...};

    memcpy(buffer + size, indices, sizeof(uint32_t) * indexCount);

    size += indexCount;

    polygonCount += 1;

    // Should be the amount of half edges there are
    edgeCount += count;
  }

  // Iteration
  uint32_t *begin() {
    return &buffer[1];
  }

  uint32_t *next(uint32_t *iterator) {
    return iterator + iterator[-1] + 1;
  }

  uint32_t *end() {
    return buffer + size + 1;
  }

  uint32_t getPolygonVertexCount(uint32_t *iterator) {
    return iterator[-1];
  }

  uint32_t getIteratorIndex(uint32_t *iterator) {
    return iterator - buffer;
  }

  uint32_t *getIteratorFromIteratorIndex(uint32_t index) {
    return &buffer[index];
  }
};

// These are all just indices into the list of half edges.
// If we need more information, we'll create a bitfield for each or something
using PolygonData = uint32_t;
using PolygonID = uint32_t;
using EdgeData = uint32_t;
using HalfEdgeID = uint32_t;

// This is an index into the mVertices array
using VertexID = uint32_t;

struct HalfEdge {
  // Don't really need anything else for our purposes
  HalfEdgeID next;
  HalfEdgeID twin;
  VertexID rootVertex;
  // Face (iterator in the polygon structure - for now into the fast polygon list)
  PolygonID polygon;
};

// For our purposes, we just need to be able to easily iterate
// over all the faces and edges of the mesh. That's it
class HalfEdgeMesh {
public:
  // Accept different formats
  void construct(
    FastPolygonList &polygons,
    uint32_t vertexCount, glm::vec3 *vertices);

  void constructCube();

  void transform(const glm::mat4 &transform);

  // Normalized
  glm::vec3 getFaceNormal(const PolygonData &polygon, glm::vec3 *vertices) const;

  // Normalized normal
  Plane getPlane(const PolygonData &polygon, glm::vec3 *vertices) const;

  // Normalized normals
  std::pair<glm::vec3, glm::vec3> getEdgeNormals(const HalfEdge &hEdge, glm::vec3 *vertices) const;

  // Normalized direction
  glm::vec3 getEdgeDirection(const EdgeData &edge, glm::vec3 *vertices) const;

  // Normalized direction
  glm::vec3 getEdgeDirection(const HalfEdge &edge, glm::vec3 *vertices) const;

  glm::vec3 getEdgeOrigin(const EdgeData &edge, glm::vec3 *vertices) const;
  glm::vec3 getEdgeOrigin(const HalfEdge &edge, glm::vec3 *vertices) const;

public:
  uint32_t getPolygonCount() const;
  const PolygonData &polygon(uint32_t id) const;

  uint32_t getEdgeCount() const;
  const EdgeData &edge(uint32_t id) const;

  const HalfEdge &halfEdge(HalfEdgeID id) const;

private:
  // For now, just array of indices which point into the half edge array
  PolygonData *mPolygons;
  uint32_t mPolygonCount;

  EdgeData    *mEdges;
  uint32_t mEdgeCount;

  // Where all the half edges are stored
  HalfEdge    *mHalfEdges;
  uint32_t mHalfEdgeCount;
};

}
