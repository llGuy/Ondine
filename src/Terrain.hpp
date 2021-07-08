#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"

namespace Ondine::Game {

class Terrain {
public:
  Terrain() = default;

  void init();

  // Creates a chunk if it doesn't exist
  Graphics::Chunk *getChunk(const glm::ivec3 &coord);
  // Doesn't create a chunk if it doesn't exist
  Graphics::Chunk *at(const glm::ivec3 &coord);

  glm::ivec3 worldToChunkCoord(const glm::vec3 &wPosition) const;
  uint32_t hashChunkCoord(const glm::ivec3 &coord) const;

private:
  static constexpr uint32_t MAX_CHUNKS = 300;

  float mTerrainScale;
  float mChunkWidth;
  Array<Graphics::Chunk *> mLoadedChunks;
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
};

}
