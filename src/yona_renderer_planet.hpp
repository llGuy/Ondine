#pragma once

#include <glm/glm.hpp>
#include "yona_vulkan_context.hpp"

namespace Yona {

/* Pushed through a push constant */
struct PlanetProperties {
  float bottomRadius;
  float topRadius;
  glm::vec3 albedo;
};

/* Only one planet to be rendered (using raytracing for a magnificent sphere) */
class RendererPlanet {
public:
  void init(VulkanContext &graphicsContext);

private:
  VulkanPipeline mPipeline;
  PlanetProperties mPlanetProperties;
};

}
