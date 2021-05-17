#pragma once

#include <glm/glm.hpp>

namespace Yona {

/* Some of the alignas are aren't necessary */

struct DensityLayer {
  alignas(4) float width;
  alignas(4) float expTerm;
  alignas(4) float expScale;
  alignas(4) float linTerm;
  alignas(4) float constTerm;
};

struct DensityProfile {
  alignas(4) DensityLayer layers[2];
};

struct SkyProperties {
  alignas(16) glm::vec3 solarIrradiance;
  alignas(4) float solarAngularRadius;
  alignas(4) float bottomRadius;
  alignas(4) float topRadius;
  alignas(4) DensityProfile rayleighDensity;
  alignas(16) glm::vec3 rayleighScatteringCoef;
  alignas(4) DensityProfile mieDensity;
  alignas(16) glm::vec3 mieScatteringCoef;
  alignas(16) glm::vec3 mieExtinctionCoef;
  alignas(4) float miePhaseFunctionG;
  alignas(4) DensityProfile absorptionDensity;
  alignas(16) glm::vec3 absorptionExtinctionCoef;
  alignas(16) glm::vec3 groundAlbedo;
  alignas(4)float muSunMin;
};

}
