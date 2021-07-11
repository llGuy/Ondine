#pragma once

#include "Chunk.hpp"
#include "VulkanPipeline.hpp"

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
    const Terrain &terrain,
    VulkanFrame &frame) const;

private:
  VulkanPipeline mPipeline;
};

}