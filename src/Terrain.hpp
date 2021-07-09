#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"

// For now everything linked to the terrain is in graphics module
namespace Ondine::Graphics {

class Terrain {
public:
  Terrain() = default;

  void init();

  // Creates a chunk if it doesn't exist
  Chunk *getChunk(const glm::ivec3 &coord);
  // Doesn't create a chunk if it doesn't exist
  Chunk *at(const glm::ivec3 &coord);
  const Chunk *at(const glm::ivec3 &coord) const;

  ChunkVertices generateChunkVertices(const Chunk &chunk);

  glm::ivec3 worldToChunkCoord(const glm::vec3 &wPosition) const;
  uint32_t hashChunkCoord(const glm::ivec3 &coord) const;
  uint32_t getVoxelIndex(const glm::ivec3 &coord) const;

private:
  Voxel getChunkEdgeVoxel(
    const glm::ivec3 &coord,
    bool *doesntExist,
    const glm::ivec3 &chunkCoord) const;

  void pushVertextoTriangleList(
    uint32_t v0, uint32_t v1,
    glm::vec3 *vertices, Voxel *voxels,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices, uint32_t &vertexCount);

private:
  static constexpr uint32_t MAX_CHUNKS = 300;
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  float mTerrainScale;
  float mChunkWidth;
  Array<Chunk *> mLoadedChunks;
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
};

}
