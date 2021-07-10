#pragma once

#include "Chunk.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::Graphics {

class GBuffer;
class Camera;
class PlanetRenderer;
class Clipping;
class VulkanContext;
struct VulkanFrame;

class TerrainRenderer {
public:
  void init(
    const GBuffer &gbuffer,
    VulkanContext &graphicsContext);

  void render(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    VulkanFrame &frame);

private:
  VulkanPipeline mPipeline;
};

}
