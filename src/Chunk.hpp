#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include "QuadTree.hpp"
#include "NumericMap.hpp"
#include "VulkanArenaSlot.hpp"

namespace Ondine::Graphics {

struct Voxel {
  // Work out what needs to be here
  uint16_t density;
  // Testing out normal generation
  int16_t normalX;
  int16_t normalY;
  int16_t normalZ;
};

extern const int8_t VOXEL_EDGE_CONNECT[256][16];

constexpr uint32_t CHUNK_DIM = 16;
constexpr int32_t INVALID_CHUNK_INDEX = -1;
constexpr uint32_t CHUNK_VOLUME = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

struct ChunkVertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct Chunk {
  Voxel voxels[CHUNK_VOLUME];

  uint32_t chunkStackIndex;
  bool needsUpdating;

  // Index of the next chunk in vertical chunk linked list
  int32_t next;

  glm::ivec3 chunkCoord;

  NumericMapKey chunkGroupKey;
};

// This is what actually gets rendered - generated with the help of a quadtree
struct ChunkGroup {
  Voxel voxels[CHUNK_VOLUME];

  VulkanArenaSlot verticesMemory;
  uint32_t vertexCount;

  glm::ivec3 coord;
  uint16_t level;
  NumericMapKey key;
  bool needsUpdate;

  glm::mat4 transform;

  QuadTree::NodeInfo nodeInfo;
};

inline uint32_t getVoxelIndex(int x, int y, int z) {
  return z * (CHUNK_DIM * CHUNK_DIM) + y * CHUNK_DIM + x;
}

inline uint32_t getVoxelIndex(const glm::ivec3 &coord) {
  return coord.z * (CHUNK_DIM * CHUNK_DIM) +
    coord.y * CHUNK_DIM + coord.x;
}

}
