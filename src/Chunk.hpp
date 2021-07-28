#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
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
constexpr uint32_t CHUNK_VOLUME = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

struct ChunkVertex {
  glm::vec3 position;
  glm::vec3 normal;
};

struct Chunk {
  Voxel voxels[CHUNK_VOLUME];
  VulkanArenaSlot verticesMemory;
  uint32_t chunkStackIndex;
  uint32_t vertexCount;
  bool needsUpdating;
  glm::ivec3 chunkCoord;
  glm::mat4 transform;
};

}
