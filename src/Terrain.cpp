#include "Terrain.hpp"

namespace Ondine::Game {

void Terrain::init() {
  mTerrainScale = 1.0f;
  mChunkWidth = mTerrainScale * (float)Graphics::CHUNK_DIM;

  mChunkIndices.init();
  mLoadedChunks.init(MAX_CHUNKS);
}

Graphics::Chunk *Terrain::getChunk(const glm::ivec3 &coord) {
  uint32_t hash = hashChunkCoord(coord);
  uint32_t &index = mChunkIndices.get(hash);
    
  if (index) {
    // Chunk was already added
    return mLoadedChunks[index];
  }
  else {
    uint32_t i = mLoadedChunks.size++;
    Graphics::Chunk *&chunk = mLoadedChunks[i];
    chunk = flAlloc<Graphics::Chunk>();

    chunk->chunkCoord = coord;
    chunk->chunkStackIndex = i;

    mChunkIndices.insert(hash, i);

    return chunk;
  }
}

Graphics::Chunk *Terrain::at(const glm::ivec3 &coord) {
  uint32_t hash = hashChunkCoord(coord);
  uint32_t &index = mChunkIndices.get(hash);

  if (index) {
    return mLoadedChunks[index];
  }
  else {
    return nullptr;
  }
}

glm::ivec3 Terrain::worldToChunkCoord(const glm::vec3 &wPosition) const {
  glm::vec3 scaled = glm::floor(wPosition / (float)Graphics::CHUNK_DIM);
  return (glm::ivec3)scaled;
}

uint32_t Terrain::hashChunkCoord(const glm::ivec3 &coord) const {
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
