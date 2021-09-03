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

struct Chunk {
  Voxel voxels[CHUNK_VOLUME];

  uint32_t chunkStackIndex;
  bool needsUpdating;

  // Index of the next chunk in vertical chunk linked list
  int32_t next;

  glm::ivec3 chunkCoord;

  NumericMapKey chunkGroupKey;
};

enum { B8_R_MAX = 0b111, B8_G_MAX = 0b111, B8_B_MAX = 0b11 };

constexpr inline glm::vec3 b8ColorToV3(uint8_t color) {
  uint8_t rb8 = color >> 5;
  uint8_t gb8 = (color >> 2) & 0b111;
  uint8_t bb8 = (color) & 0b11;

  float rf32 = (float)(rb8) / (float)(B8_R_MAX);
  float gf32 = (float)(gb8) / (float)(B8_G_MAX);
  float bf32 = (float)(bb8) / (float)(B8_B_MAX);

  return glm::vec3(rf32, gf32, bf32);
}

constexpr inline uint8_t v3ColorToB8(const glm::vec3 &color) {
  float r = color.r * (float)(B8_R_MAX);
  float g = color.g * (float)(B8_G_MAX);
  float b = color.b * (float)(B8_B_MAX);

  return ((uint8_t)r << 5) + ((uint8_t)g << 2) + ((uint8_t)b);
}

constexpr inline uint8_t b8vColorToB8(uint8_t r, uint8_t g, uint8_t b) {
  return (r << 5) + (g << 2) + b;
}

constexpr inline uint32_t getVoxelIndex(int x, int y, int z) {
  return z * (CHUNK_DIM * CHUNK_DIM) + y * CHUNK_DIM + x;
}

constexpr inline uint32_t getVoxelIndex(const glm::ivec3 &coord) {
  return coord.z * (CHUNK_DIM * CHUNK_DIM) +
    coord.y * CHUNK_DIM + coord.x;
}

inline uint32_t hashFlatChunkCoord(const glm::ivec2 &coord) {
  struct {
    union {
      struct {
        uint32_t p: 1;
        uint32_t x: 15;
        uint32_t z: 16;
      };
      uint32_t value;
    };
  } hasher;

  hasher.value = 0;

  hasher.x = *(uint32_t *)(&coord.x);
  hasher.z = *(uint32_t *)(&coord.y);

  return (uint32_t)hasher.value;
}

inline int32_t hashChunkCoord(const glm::ivec3 &coord) {
  struct {
    union {
      struct {
        uint32_t padding: 2;
        uint32_t x: 10;
        uint32_t y: 10;
        uint32_t z: 10;
      };
      uint32_t value;
    };
  } hasher;

  hasher.value = 0;

  hasher.x = *(uint32_t *)(&coord.x);
  hasher.y = *(uint32_t *)(&coord.y);
  hasher.z = *(uint32_t *)(&coord.z);

  return (uint32_t)hasher.value;
}

}
