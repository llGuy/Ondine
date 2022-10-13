#pragma once

#include "Chunk.hpp"
#include "FastMap.hpp"
#include "QuadTree.hpp"
#include "VulkanContext.hpp"
#include "VulkanArenaAllocator.hpp"

#include <glm/glm.hpp>

namespace Ondine::Graphics {

/* Contains raw voxel information */
class Terrain;

struct IsoVertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
};

/* A group which can encompass a certain volume of chunks depending on LOD */
struct IsoGroup {
  Voxel voxels[CHUNK_VOLUME];

  VulkanArenaSlot vertices;
  uint32_t vertexCount;
  IsoVertex *verticesMem;

  VulkanArenaSlot transVoxelVertices;
  uint32_t transVoxelVertexCount;
  IsoVertex *transVerticesMem;

  glm::ivec3 coord;
  uint16_t level;
  NumericMapKey key;

  union {
    struct {
      uint8_t needsUpdate : 1;
      uint8_t pushedToFullUpdates : 1;
      uint8_t pushedToTransitionUpdates : 1;
      uint8_t pad : 5;
    };

    uint8_t bits;
  };

  int32_t next;

  glm::mat4 transform;

  QuadTree::NodeInfo nodeInfo;
};

/* This is what the IsoSurfaceRenderer will be rendering from */
struct IsoGroupSnapshot {
  glm::ivec3 coord;
  uint16_t level;
  uint32_t vertexCount, transVertexCount;
  VulkanArenaSlot vertices, transVertices;
};

/* Creates isosurface mesh from voxel chunks and quad tree */
class Isosurface {
public:
  void init(const QuadTree &quadTree, VulkanContext &graphicsContext);

  /* Adds all updated chunks to full updates, or transition updates */
  void prepareForUpdate(QuadTree &quadTree, Terrain &terrain);

  /* Make sure that the terrain scale is right */
  void bindToTerrain(const Terrain &terrain);

  void syncWithGPU(const VulkanCommandBuffer &commandBuffer);

  /* These doesn't belong here whatsoever */
  glm::vec2 worldToQuadTreeCoords(
    const QuadTree &quadTree,
    glm::vec2 offset) const;

  glm::ivec2 quadTreeCoordsToWorld(
    const QuadTree &quadTree,
    glm::ivec2 offset) const;

  NumericMap<IsoGroup *> &isoGroups();

private:
  glm::ivec3 getIsoGroupCoord(
    const QuadTree &quadTree,
    const glm::ivec3 &chunkCoord) const;
  uint32_t hashIsoGroupCoord(const glm::ivec3 &coord) const;

  void addToFlatIsoGroupIndices(IsoGroup *isoGroup);
  IsoGroup *getFirstFlatIsoGroup(glm::ivec2 flatCoord);

  // One unit in offset = chunk coord. The origin of the quadtree is at 0,0
  glm::ivec2 quadTreeCoordsToChunk(
    const QuadTree &quadTree,
    glm::ivec2 offset) const;

  IsoGroup *getIsoGroup(const glm::ivec3 &coord);
  void freeIsoGroup(IsoGroup *group);

  struct GenIsoGroupVerticesParams {
    const Terrain &terrain;
    const QuadTree &quadTree;
    const IsoGroup &group;
  };

  uint32_t generateVertices(GenIsoGroupVerticesParams, IsoVertex *out);
  uint32_t generateTransVoxelVertices(GenIsoGroupVerticesParams, IsoVertex *out);

  void updateIsoGroupFace(
    GenIsoGroupVerticesParams params,
    uint32_t primaryAxis, uint32_t secondAxis,
    uint32_t faceAxis, uint32_t side,
    IsoVertex *meshVertices, uint32_t &vertexCount);

  void updateVoxelCell(
    Voxel *voxels,
    const glm::ivec3 &coord,
    IsoVertex *meshVertices,
    uint32_t &vertexCount);

  void updateTransVoxelCell(
    Voxel *voxels,
    Voxel *transVoxels,
    const glm::ivec3 &axis,
    const glm::ivec3 &coord,
    IsoVertex *meshVertices,
    uint32_t &vertexCount);

private:
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  /* This is where vertices will get allocated on the GPU */
  VulkanArenaAllocator mGPUVerticesAllocator;

  /* After every update, this gets filled up with vertices */
  IsoVertex *mVertexPool;

  /* IsoGroups which need a full update - both inner and outer voxels */
  IsoGroup **mFullUpdates;
  uint32_t mFullUpdateCount;

  /* IsoGroups which just need outer voxels updated (next to LOD change) */
  IsoGroup **mTransitionUpdates;
  uint32_t mTransitionUpdateCount;

  /* Where the actual IsoGroups are stored */
  NumericMap<IsoGroup *> mIsoGroups;
  FastMap<uint32_t, 1000, 30, 10> mIsoGroupIndices;
  FastMap<uint32_t, 500, 30, 10> mFlatIsoGroupIndices;

  Voxel mSurfaceDensity;

  int mScale;

  friend class View::EditorView;
};

}
