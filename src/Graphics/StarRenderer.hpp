#pragma once

#include "Tick.hpp"
#include <stdint.h>
#include "Model.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include "TrackedResource.hpp"

namespace Ondine::Graphics {

struct LightingProperties;

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

  void render(
    float starSize,
    const Camera &camera, VulkanFrame &frame) const;

  void tick(
    const CameraProperties &camera,
    const LightingProperties &lighting,
    const PlanetProperties &planet,
    const Core::Tick &tick);
  
  void generateStars();
  
private:
  static const char *const STARS_VERT_SPV;
  static const char *const STARS_FRAG_SPV;

  struct PushConstant {
    glm::mat4 transform;
    float fade;
    float starSize;
  } mPushConstant;

  uint32_t mStarCount;
  Array<Star> mStars;

  ModelConfig mPipelineModelConfig;
  Model mStarsModel;
  TrackedResource<VulkanPipeline, StarRenderer> mPipeline;
  float mCurrentRotation;

  // Not very nice - need to find workaround to this
  const RenderStage *mGBuffer;
};

}
