#pragma once

#include <glm/glm.hpp>

namespace Yona {

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

struct SkyProperties {
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

}
