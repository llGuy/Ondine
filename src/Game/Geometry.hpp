#pragma once

#include "Math.hpp"
#include "Memory.hpp"

namespace Ondine::Game::Physics {

struct FastPolygonList {
  uint32_t *buffer;
  uint32_t size;
  uint32_t edgeCount;
  uint32_t polygonCount;

  void allocate(uint32_t maxIndices) {
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
    return buffer + size;
  }

  uint32_t getPolygonVertexCount(uint32_t *iterator) {
    return iterator[-1];
  }
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

  // void construct(/* some other format */);

private:
  // This is an index into the mVertices array
  using VertexID = uint32_t;

  // These are all just indices into the list of half edges.
  // If we need more information, we'll create a bitfield for each or something
  using PolygonData = uint32_t;
  using EdgeData = uint32_t;
  using HalfEdgeID = uint32_t;

  struct HalfEdge {
    // Don't really need anything else for our purposes
    HalfEdgeID next;
    HalfEdgeID twin;
    VertexID rootVertex;
  };

  // Where actual vertices get stored
  glm::vec3   *mVertices;
  uint32_t mVertexCount;

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
