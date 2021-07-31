#pragma once

#include <glm/glm.hpp>
#include "QuadTree.hpp"
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
  VulkanArenaAllocator mGPUVerticesAllocator;
  // For debugging purposes
  VulkanPipeline mRenderLine;
};

}
