#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"

/* 
   For now everything linked to the terrain is in graphics module
   and also, everything is implemented very badly
*/
namespace Ondine::Graphics {

class VulkanContext;
class Camera;
class PlanetRenderer;
class Clipping;
struct VulkanFrame;

class Terrain {
public:
  Terrain() = default;

  void init();

  void prepareForRender(VulkanContext &graphicsContext);

  void submitForRender(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    VulkanFrame &frame);

  void makeSphere(float radius, const glm::vec3 &center);

  // TODO: Get buffers from a pool
  ChunkVertices createChunkVertices(
    const Chunk &chunk,
    VulkanContext &graphicsContext);

  // Creates a chunk if it doesn't exist
  Chunk *getChunk(const glm::ivec3 &coord);
  // Doesn't create a chunk if it doesn't exist
  Chunk *at(const glm::ivec3 &coord);
  const Chunk *at(const glm::ivec3 &coord) const;

  glm::ivec3 worldToChunkCoord(const glm::vec3 &wPosition) const;
  glm::vec3 chunkCoordToWorld(const glm::ivec3 &chunkCoord) const;
  uint32_t hashChunkCoord(const glm::ivec3 &coord) const;
  uint32_t getVoxelIndex(const glm::ivec3 &coord) const;
  uint32_t getVoxelIndex(int x, int y, int z) const;

private:
  uint32_t generateChunkVertices(
    const Chunk &chunk,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices);

  Voxel getChunkEdgeVoxel(
    int x, int y, int z,
    bool *doesntExist,
    const glm::ivec3 &chunkCoord) const;

  void pushVertexToTriangleList(
    uint32_t v0, uint32_t v1,
    glm::vec3 *vertices, Voxel *voxels,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices, uint32_t &vertexCount);

  void updateVoxelCube(
    Voxel *voxels,
    const glm::ivec3 &coord,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices,
    uint32_t &vertexCount);

private:
  static constexpr uint32_t MAX_CHUNKS = 300;
  static constexpr uint32_t CHUNK_MAX_VERTICES =
    5 * (CHUNK_DIM - 1) * (CHUNK_DIM - 1) * (CHUNK_DIM - 1);
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  float mTerrainScale;
  float mChunkWidth;
  Array<Chunk *> mLoadedChunks;
  // Maps 3-D chunk coord to the chunk's index in the mLoadedChunks array
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
  ChunkVertex *mTemporaryVertices;

  friend class TerrainRenderer;
};

}
