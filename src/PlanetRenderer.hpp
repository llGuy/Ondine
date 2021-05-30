#pragma once

#include "IO.hpp"
#include <glm/glm.hpp>
#include "VulkanPipeline.hpp"

namespace Yona {

struct Tick;
struct VulkanFrame;

class Camera;
class RenderStage;
class VulkanContext;

/* Some of the alignas are aren't necessary */
struct DensityLayer {
  float width;
  float expTerm;
  float expScale;
  float linTerm;
  float constTerm;

  float pad[3];
};

struct DensityProfile {
  alignas(16) DensityLayer layers[2];
};

struct PlanetProperties {
  alignas(16) glm::vec3 solarIrradiance;
  alignas(4) float solarAngularRadius;
  alignas(4) float bottomRadius;
  alignas(4) float topRadius;
  alignas(16) DensityProfile rayleighDensity;
  alignas(16) glm::vec3 rayleighScatteringCoef;
  alignas(16) DensityProfile mieDensity;
  alignas(16) glm::vec3 mieScatteringCoef;
  alignas(16) glm::vec3 mieExtinctionCoef;
  alignas(4) float miePhaseFunctionG;
  alignas(16) DensityProfile absorptionDensity;
  alignas(16) glm::vec3 absorptionExtinctionCoef;
  alignas(16) glm::vec3 groundAlbedo;
  alignas(4) float muSunMin;
};

/* Only one planet to be rendered (using raytracing for a magnificent sphere) */
class PlanetRenderer {
public:
  void init(
    VulkanContext &graphicsContext,
    const RenderStage &renderStage);

  void tick(
    const Tick &tick,
    VulkanFrame &frame,
    Resolution viewport,
    const Camera &camera);

private:
  VulkanPipeline mPipeline;
  PlanetProperties mPlanetProperties;
};

}
