#pragma once

#include "Chunk.hpp"
#include <glm/glm.hpp>
#include "FastMap.hpp"
#include "QuadTree.hpp"
#include "NumericMap.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::View {

class EditorView;
class MapView;

}

namespace Ondine::Graphics {

class GBuffer;
class Camera;
class PlanetRenderer;
class Clipping;
class Terrain;
class VulkanContext;
struct VulkanFrame;
struct CameraProperties;

class TerrainRenderer {
public:
  void init(
    VulkanContext &graphicsContext,
    const GBuffer &gbuffer);

  void render(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderWireframe(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderChunkOutlines(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderQuadTree(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void sync(
    Terrain &terrain,
    const CameraProperties &camera,
    const VulkanCommandBuffer &commandBuffer);

private:
  glm::ivec3 getChunkGroupCoord(
    const Terrain &terrain,
    const glm::ivec3 &chunkCoord) const;
  uint32_t hashChunkGroupCoord(
    const glm::ivec3 &coord) const;

  ChunkGroup *getChunkGroup(const glm::ivec3 &coord);
  void freeChunkGroup(ChunkGroup *group);

  uint32_t generateVertices(
    const Terrain &terrain,
    const ChunkGroup &group,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices);

  uint32_t generateTransVoxelVertices(
    const Terrain &terrain,
    const ChunkGroup &group,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices);

  void updateChunkFace(
    const Terrain &terrain,
    const ChunkGroup &group,
    Voxel surfaceDensity,
    uint32_t primaryAxis, uint32_t secondAxis,
    uint32_t faceAxis, uint32_t side,
    ChunkVertex *meshVertices, uint32_t &vertexCount);

  void updateVoxelCell(
    Voxel *voxels,
    const glm::ivec3 &coord,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices,
    uint32_t &vertexCount);

  void updateTransVoxelCell(
    Voxel *voxels,
    Voxel *transVoxels,
    const glm::ivec3 &axis,
    const glm::ivec3 &coord,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices,
    uint32_t &vertexCount);

private:
  void renderQuadTreeNode(
    QuadTree::Node *node,
    const glm::ivec2 &offset,
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  // One unit in offset = chunk coord. The origin of the quadtree is at 0,0
  glm::ivec2 quadTreeCoordsToChunk(glm::ivec2 offset) const;
  glm::ivec2 quadTreeCoordsToWorld(const Terrain &, glm::ivec2 offset) const;
  glm::vec2 worldToQuadTreeCoords(const Terrain &, glm::vec2 offset) const;

  void addToFlatChunkGroupIndices(ChunkGroup *chunkGroup);
  ChunkGroup *getFirstFlatChunkGroup(glm::ivec2 flatCoord);

private:
  static const glm::vec3 NORMALIZED_CUBE_VERTICES[8];
  static const glm::ivec3 NORMALIZED_CUBE_VERTEX_INDICES[8];

  VulkanPipeline mPipeline;
  VulkanPipeline mPipelineWireframe;
  VulkanArenaAllocator mGPUVerticesAllocator;
  // For debugging purposes
  VulkanPipeline mRenderLine;

  NumericMap<ChunkGroup *> mChunkGroups;
  FastMap<uint32_t, 1000, 30, 10> mChunkGroupIndices;
  FastMap<uint32_t, 500, 30, 10> mFlatChunkGroupIndices;

  ChunkVertex *mTemporaryVertices;
  ChunkVertex *mTemporaryTransVertices;
  ChunkGroup *mNullChunkGroup;
  
  QuadTree mQuadTree;

  friend class View::EditorView;
  friend class View::MapView;
};

}
