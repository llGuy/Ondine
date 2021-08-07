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
  VulkanPipeline mPipeline;
  VulkanPipeline mPipelineWireframe;
  VulkanArenaAllocator mGPUVerticesAllocator;
  // For debugging purposes
  VulkanPipeline mRenderLine;

  NumericMap<ChunkGroup *> mChunkGroups;
  FastMap<uint32_t, 1000, 30, 10> mChunkGroupIndices;
};

}
