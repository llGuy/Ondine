#pragma once

#include "Buffer.hpp"
#include "Camera.hpp"
#include "NumericMap.hpp"
#include "RenderStage.hpp"
#include "TrackedResource.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

namespace Ondine::View {

class DemoView;

}

namespace Ondine::Graphics {

struct Particle {
  glm::vec2 dstPosition;
  glm::vec2 currentPosition;
  glm::vec2 velocity;
  glm::vec4 color;
  float random;

  void init(float circleRadius) {
    random = glm::linearRand(0.0f, 1.0f);

    dstPosition = circleRadius * glm::vec2(
      glm::cos(glm::radians(random * 360.0f)),
      glm::sin(glm::radians(random * 360.0f)));
    currentPosition = dstPosition +
      glm::linearRand(glm::vec2(-1.0f), glm::vec2(1.0f)) * 0.01f;

    color = glm::vec4(1.8f, 0.9f, 2.85f, 1.0f) * 5.0f;
    velocity = glm::vec2(0.0f);
  }
};

class ParticleDemo : public ResourceTracker {
public:
  ParticleDemo() = default;

  void init(
    uint32_t particleCount,
    const RenderStage &gbuffer,
    VulkanContext &graphicsContext);

  void tick(
    const Camera &camera,
    const Core::Tick &tick, Graphics::VulkanFrame &frame);

  void render(
    const Camera &camera,
    const Core::Tick &tick, Graphics::VulkanFrame &frame);

public:
  // This is absolutely terrible. Should be using a vertex buffer but ALAS
  struct PushConstant {
    glm::vec4 color;
  };

  struct ComputePushConstant {
    glm::vec4 color;
    int circleCount;
    int particleCount;
    float dt;
  };

private:
  // Array<Particle> mParticles;
  NumericMap<glm::vec2> mCircles;

  TrackedResource<VulkanPipeline, ParticleDemo> mCompute;
  TrackedResource<VulkanPipeline, ParticleDemo> mPipeline;

  VulkanBuffer mParticleBuffers[4];
  VulkanBuffer mCircleBuffer;

  VulkanUniform mParticleUniform;
  VulkanUniform mCircleUniform;

  const RenderStage *mGBuffer;

  friend class View::DemoView;
};

}
