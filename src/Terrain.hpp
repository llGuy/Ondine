#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "FastMap.hpp"
#include "QuadTree.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::View {

class EditorView;

}

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

  void makeSphere(float radius, glm::vec3 center, float intensity = 1.0f);
  void makeIslands(
    float seaLevel, uint32_t octaveCount,
    float persistance, float lacunarity,
    float baseAmplitude, float baseFrequency,
    glm::vec2 s, glm::vec2 e);
  void paint(
    glm::vec3 position,
    glm::vec3 direction,
    float radius,
    float strength);

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
  uint32_t hashFlatChunkCoord(const glm::ivec2 &coord) const;
  uint32_t getVoxelIndex(const glm::ivec3 &coord) const;
  uint32_t getVoxelIndex(int x, int y, int z) const;

private:
  void generateChunkGroups();

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

  void markChunkForUpdate(Chunk *chunk);

  // One unit in offset = chunk coord. The origin of the quadtree is at 0,0
  glm::ivec2 quadTreeCoordsToChunk(glm::ivec2 offset) const;
  glm::ivec2 quadTreeCoordsToWorld(glm::ivec2 offset) const;
  glm::vec2 worldToQuadTreeCoords(glm::vec2 offset) const;

  void addToFlatChunkIndices(Chunk *chunk);
  Chunk *getFirstFlatChunk(glm::ivec2 flatCoord);

private:
  static constexpr uint32_t MAX_DENSITY = 0xFFFF;
  static constexpr uint32_t MAX_CHUNKS = 3000;
  static constexpr uint32_t CHUNK_MAX_VERTICES =
    10 * (CHUNK_DIM) * (CHUNK_DIM) * (CHUNK_DIM);
  static constexpr uint32_t SURFACE_DENSITY = 30000;
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  int mTerrainScale;
  float mChunkWidth;
  float mMaxVoxelDensity;
  Chunk *mNullChunk;
  Array<Chunk *> mLoadedChunks;
  Array<uint32_t> mUpdatedChunks;
  // Maps 3-D chunk coord to the chunk's index in the mLoadedChunks array
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
  // Points to a linked list of chunks all of which are at a certain x-z
  FastMap<uint32_t, MAX_CHUNKS / 2, 30, 10> mFlatChunkIndices;

  ChunkVertex *mTemporaryVertices;
  bool mUpdated;
  QuadTree mQuadTree;

  friend class TerrainRenderer;
  friend class View::EditorView;
};

}
