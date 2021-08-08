#pragma once

#include "Chunk.hpp"
#include <glm/glm.hpp>
#include "FastMap.hpp"
#include "QuadTree.hpp"
#include "NumericMap.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::Graphics {

class GBuffer;
class Camera;
class PlanetRenderer;
class Clipping;
class Terrain;
class VulkanContext;
struct VulkanFrame;

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
    const VulkanCommandBuffer &commandBuffer);

private:
  glm::ivec3 getChunkGroupCoord(
    const Terrain &terrain,
    const glm::ivec3 &chunkCoord) const;
  uint32_t hashChunkGroupCoord(
    const glm::ivec3 &coord) const;

  ChunkGroup *getChunkGroup(const glm::ivec3 &coord);

  uint32_t generateVertices(
    const ChunkGroup &group,
    Voxel surfaceDensity,
    ChunkVertex *meshVertices);

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
  void renderQuadTreeNode(
    QuadTree::Node *node,
    const glm::ivec2 &offset,
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

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

  ChunkVertex *mTemporaryVertices;
};

}
