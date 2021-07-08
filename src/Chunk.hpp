#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

namespace Ondine::Graphics {

struct Voxel {
  // Work out what needs to be here
  uint32_t bits;
};

constexpr uint32_t CHUNK_DIM = 32;
constexpr uint32_t CHUNK_VOLUME = CHUNK_DIM * CHUNK_DIM * CHUNK_DIM;

struct Chunk {
  Voxel voxels[CHUNK_VOLUME];
  glm::ivec3 chunkCoord;
  uint32_t chunkStackIndex;
};

}
