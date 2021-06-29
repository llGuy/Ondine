#pragma once

#include <stdint.h>
#include "Model.hpp"
#include "Buffer.hpp"
#include "GBuffer.hpp"
#include "VulkanContext.hpp"
#include "TrackedResource.hpp"

namespace Ondine::Graphics {

struct Star {
  float azimuthAngleRadians;
  float zenithAngleRadians;
  float brightness;
};

/* 
   Not using a skybox/dome allows us to have more control over the stars
   like their sparkliness, etc...
*/
class StarRenderer : public ResourceTracker {
public:
  StarRenderer();

  void init(
    VulkanContext &graphicsContext,
    const GBuffer &gbuffer,
    uint32_t starCount);

  void generateStars();
  
private:
  static const char *const STARS_VERT_SPV;
  static const char *const STARS_FRAG_SPV;

  uint32_t mStarCount;
  Array<Star> mStars;

  ModelConfig mPipelineModelConfig;
  TrackedResource<VulkanPipeline, StarRenderer> mPipeline;

  // Not very nice - need to find workaround to this
  const RenderStage *mGBuffer;
};

}
