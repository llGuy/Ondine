#include "GBuffer.hpp"
#include "ParticleDemo.hpp"
#include <glm/gtc/random.hpp>

namespace Ondine::Graphics {

void ParticleDemo::init(
  uint32_t particleCount,
  const RenderStage &gbuffer,
  VulkanContext &graphicsContext) {
  /* Initialize particles and circles */
  mParticles.init(particleCount);
  mCircles.init(MAX_CIRCLE_COUNT);

  float firstCircleRadius = 0.5f;

  mCircles.add(Circle{glm::vec2(0.0f), firstCircleRadius});

  for (int i = 0; i < mParticles.capacity; ++i) {
    mParticles[i].init(firstCircleRadius);
  }

  /* initialize particle rendering resources */
  addTrackedPath("res/spv/Point.vert.spv", &mPipeline);
  addTrackedPath("res/spv/Point.frag.spv", &mPipeline);

  mGBuffer = &gbuffer;

  mPipeline.init(
    [] (
      VulkanPipeline &res,
      ParticleDemo &owner,
      VulkanContext &graphicsContext) {
      VulkanPipelineConfig pipelineConfig(
        {owner.mGBuffer->renderPass(), 0},
        VulkanShader{graphicsContext.device(), "res/spv/Point.vert.spv"},
        VulkanShader{graphicsContext.device(), "res/spv/Point.frag.spv"});

      pipelineConfig.configurePipelineLayout(
        sizeof(ParticleDemo::PushConstant),
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

      pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

      res.init(
        graphicsContext.device(),
        graphicsContext.descriptorLayouts(),
        pipelineConfig);
    },
    *this,
    graphicsContext);
}

void ParticleDemo::tick(
  const Camera &camera,
  const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(camera.uniform());

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  float radius = 0.5f / (float)mCircles.size();

  int particleIdx = 0;
  int particlesPerCircle = 1 + mParticles.capacity / mCircles.size();

  for (auto &circle : mCircles) {
    int startIdx = particleIdx;
    int endIdx = particleIdx + particlesPerCircle;

    for (; particleIdx < endIdx && particleIdx < mParticles.capacity; ++particleIdx) {
      auto &particle = mParticles[particleIdx];

      float circleProgress = particle.random;

      particle.dstPosition = circle.center + radius * glm::vec2(
        glm::cos(glm::radians(circleProgress * 360.0f)),
        glm::sin(glm::radians(circleProgress * 360.0f)));

      glm::vec2 diff = (particle.dstPosition - particle.currentPosition);
      float magSq = glm::dot(diff, diff);

      glm::vec2 acc = diff * magSq;

      particle.velocity += acc * tick.dt;
      particle.currentPosition += particle.velocity * tick.dt;

      PushConstant pushConstant = {
        glm::vec4(
          particle.currentPosition.x,
          particle.currentPosition.y,
          0.0f, 1.0f),

        particle.color,
        1.0f,
        3.0f
      };

      commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

      commandBuffer.draw(1, 1, 0, 0);
    }
  }
}

}
