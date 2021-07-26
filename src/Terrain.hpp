#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"
#include "VulkanArenaAllocator.hpp"

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

  void makeSphere(float radius, glm::vec3 center);
  void makeIslands(
    float seaLevel, uint32_t octaveCount,
    float persistance, float lacunarity,
    float baseAmplitude, float baseFrequency,
    glm::vec2 s, glm::vec2 e);

  // TODO: Get buffers from a pool
  ChunkVertex *createChunkVertices(
    const Chunk &chunk,
    uint32_t *vertexCount);

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

  void generateVoxelNormals();

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

  template <typename Proc>
  void apply3D(int start, int end, Proc applyProc) {
    for (int z = start; z < end; ++z) {
      for (int y = start; y < end; ++y) {
        for (int x = start; x < end; ++x) {
          applyProc(glm::ivec3(x, y, z));
        }
      }
    }
  }

  template <typename Proc>
  void apply2D(int start, int end, Proc applyProc) {
    for (int y = start; y < end; ++y) {
      for (int x = start; x < end; ++x) {
        applyProc(glm::ivec2(x, y));
      }
    }
  }

  void generateChunkFaceNormals(
    Chunk *chunk,
    Chunk *p, Chunk *n,
    uint32_t dimension);

  void setVoxelNormal(
    Chunk *chunk,
    const glm::ivec3 &voxelCoord,
    const glm::vec3 &grad);

private:
  static constexpr uint32_t MAX_CHUNKS = 3000;
  static constexpr uint32_t CHUNK_MAX_VERTICES =
    10 * (CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM);
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  int mTerrainScale;
  float mChunkWidth;
  float mMaxVoxelDensity;
  Chunk *mNullChunk;
  Array<Chunk *> mLoadedChunks;
  // Maps 3-D chunk coord to the chunk's index in the mLoadedChunks array
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
  ChunkVertex *mTemporaryVertices;
  bool mUpdated;

  friend class TerrainRenderer;
};

}
