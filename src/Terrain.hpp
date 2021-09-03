#pragma once

#include "Chunk.hpp"
#include "Buffer.hpp"
#include "Worker.hpp"
#include "FastMap.hpp"
#include "QuadTree.hpp"
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

  void makeSphere(float radius, glm::vec3 center, float intensity = 1.0f);
  void makeIslands(
    float seaLevel, uint32_t octaveCount,
    float persistance, float lacunarity,
    float baseAmplitude, float baseFrequency,
    glm::vec2 s, glm::vec2 e);
  void makePlane(float radius, glm::vec3 center, float intensity = 1.0f);

  void paint(
    glm::vec3 position,
    glm::vec3 direction,
    float radius,
    float strength);

  void queuePaint(
    glm::vec3 position,
    glm::vec3 direction,
    float radius,
    float strength);

  // Creates a chunk if it doesn't exist
  Chunk *getChunk(const glm::ivec3 &coord);
  // Doesn't create a chunk if it doesn't exist
  Chunk *at(const glm::ivec3 &coord);
  const Chunk *at(const glm::ivec3 &coord) const;

  glm::ivec3 worldToChunkCoord(const glm::vec3 &wPosition) const;
  glm::vec3 chunkCoordToWorld(const glm::ivec3 &chunkCoord) const;

  // position isn't scaled by mTerrainScale
  const Voxel &getVoxel(const glm::vec3 &position) const;

  void generateVoxelNormals();

private:
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
  void addToFlatChunkIndices(Chunk *chunk);
  Chunk *getFirstFlatChunk(glm::ivec2 flatCoord) const;

  enum TerrainModificationType {
    DensityPaint,
    ColorPaint,
    AddSphere
    /* ... */
  };

  struct TerrainModificationParams {
    Terrain *terrain;
    TerrainModificationType type;

    union {
      struct {
        glm::vec3 rayStart;
        glm::vec3 rayDirection;
        float radius;
        float strength;
      } dp;

      struct {
        glm::vec3 rayStart;
        glm::vec3 rayDirection;
      } cp;

      struct {
        glm::vec3 centre;
        float radius;
        // Color information
      } as;
    };
  };

  static int runTerrainModification(void *data);

private:
  static constexpr uint32_t MAX_DENSITY = 0xFFFF;
  static constexpr uint32_t MAX_CHUNKS = 3000;
  static constexpr uint32_t SURFACE_DENSITY = 30000;

  int mTerrainScale;
  float mChunkWidth;
  float mMaxVoxelDensity;
  Chunk *mNullChunk;
  Array<Chunk *> mLoadedChunks;
  Array<uint32_t> mUpdatedChunks;
  // Maps 3-D chunk coord to the chunk's index in the mLoadedChunks array
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mChunkIndices;
  // Points to a linked list of chunks all of which are at a certain x-z
  FastMap<uint32_t, MAX_CHUNKS, 30, 10> mFlatChunkIndices;

  Core::JobID mModificationJob;
  TerrainModificationParams *mModificationParams;

  bool mUpdated;
  // The terrain renderer may toggle this if it decides to update isogroups
  bool mLockedActionQueue;

  friend class TerrainRenderer;
  friend class Isosurface;
};

}
