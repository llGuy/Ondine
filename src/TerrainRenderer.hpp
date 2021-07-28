#pragma once

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

  void sync(
    Terrain &terrain,
    const VulkanCommandBuffer &commandBuffer);

private:
  VulkanPipeline mPipeline;
  VulkanArenaAllocator mGPUVerticesAllocator;
  // For debugging purposes
  VulkanPipeline mRenderLine;
};

}
