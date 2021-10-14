#include "GBuffer.hpp"
#include "ParticleDemo.hpp"
#include <glm/gtc/random.hpp>

namespace Ondine::Graphics {

static constexpr uint32_t MAX_CIRCLE_COUNT = 100;
static constexpr uint32_t PARTICLE_COUNT = 1000000;

void ParticleDemo::init(
  uint32_t particleCount,
  const RenderStage &gbuffer,
  VulkanContext &graphicsContext) {
  /* Initialize particles and circles */
  // mParticles.init(particleCount);
  mCircles.init(MAX_CIRCLE_COUNT);

  float firstCircleRadius = 0.5f;

  mCircles.add(glm::vec2(0.0f));

  /*
  for (int i = 0; i < PARTICLE_COUNT; ++i) {
    mParticles[i].init(firstCircleRadius);
  }
  */

  /* Initialize compute pipeline */
  mCompute.init(
    [] (
      VulkanPipeline &res,
      ParticleDemo &owner,
      VulkanContext &graphicsContext) {
      VulkanPipelineConfig pipelineConfig(
        {NullReference<VulkanRenderPass>::nullRef, 0},
        VulkanShader{graphicsContext.device(), "res/spv/Particle.comp.spv"});

      pipelineConfig.configurePipelineLayout(
        sizeof(ParticleDemo::ComputePushConstant),
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});

      res.init(
        graphicsContext.device(),
        graphicsContext.descriptorLayouts(),
        pipelineConfig);
    },
    *this,
    graphicsContext);

  // 0: positions, 1: dstPositions, 2: velocities, 3: random
  float *tmp1f = new float[PARTICLE_COUNT];
  glm::vec2 *tmp2f = new glm::vec2[PARTICLE_COUNT];



  // Random numbers
  float *randomPtr = tmp1f;
  for (int i = 0; i < PARTICLE_COUNT; ++i) {
    randomPtr[i] = glm::linearRand(0.0f, 1.0f);
  }

  mParticleBuffers[3].init(
    graphicsContext.device(), sizeof(float) * PARTICLE_COUNT,
    (VulkanBufferFlagBits)VulkanBufferFlag::StorageBuffer);
  mParticleBuffers[3].fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)tmp1f, sizeof(float) * PARTICLE_COUNT});



  // Destination position
  for (int i = 0; i < PARTICLE_COUNT; ++i) {
    tmp2f[i] = firstCircleRadius * glm::vec2(
      glm::cos(glm::radians(randomPtr[i] * 360.0f)),
      glm::sin(glm::radians(randomPtr[i] * 360.0f)));
  }
  mParticleBuffers[1].init(
    graphicsContext.device(), sizeof(glm::vec2) * PARTICLE_COUNT,
    (VulkanBufferFlagBits)VulkanBufferFlag::StorageBuffer);
  mParticleBuffers[1].fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)tmp2f, sizeof(glm::vec2) * PARTICLE_COUNT});


  for (int i = 0; i < PARTICLE_COUNT; ++i) {
    tmp2f[i] = tmp2f[i] + 
      glm::linearRand(glm::vec2(-1.0f), glm::vec2(1.0f)) * 0.01f;
  }
  mParticleBuffers[0].init(
    graphicsContext.device(), sizeof(glm::vec2) * PARTICLE_COUNT,
    VulkanBufferFlag::StorageBuffer | VulkanBufferFlag::VertexBuffer);
  mParticleBuffers[0].fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)tmp2f, sizeof(glm::vec2) * PARTICLE_COUNT});


  memset(tmp2f, 0, sizeof(glm::vec2) * PARTICLE_COUNT);
  mParticleBuffers[2].init(
    graphicsContext.device(), sizeof(glm::vec2) * PARTICLE_COUNT,
    (VulkanBufferFlagBits)VulkanBufferFlag::StorageBuffer);
  mParticleBuffers[2].fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)tmp2f, sizeof(glm::vec2) * PARTICLE_COUNT});


  mParticleUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(
      mParticleBuffers[0], mParticleBuffers[1], mParticleBuffers[2], mParticleBuffers[3]),
    true);
  

  // Circle buffer
  mCircleBuffer.init(
    graphicsContext.device(), sizeof(glm::vec2) * MAX_CIRCLE_COUNT,
    (VulkanBufferFlagBits)VulkanBufferFlag::StorageBuffer);
  mCircleBuffer.fillWithStaging(
    graphicsContext.device(), graphicsContext.commandPool(),
    {(uint8_t *)&mCircles[0], sizeof(glm::vec2) * MAX_CIRCLE_COUNT});

  mCircleUniform.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanBuffer, AllocationType::Linear>(mCircleBuffer),
    true);
  

  /* Initialize particle rendering resources */
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

      pipelineConfig.configureVertexInput(1, 1);
      pipelineConfig.setBinding(0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX);
      pipelineConfig.setBindingAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);

      res.init(
        graphicsContext.device(),
        graphicsContext.descriptorLayouts(),
        pipelineConfig);
    },
    *this,
    graphicsContext);
}

void ParticleDemo::render(
  const Camera &camera,
  const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;
  
  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(camera.uniform());
  commandBuffer.bindVertexBuffers(0, 1, &mParticleBuffers[0]);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  PushConstant pushConstant = {
    glm::vec4(1.8f, 0.9f, 2.85f, 1.0f) * 5.0f
  };

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);
}

void ParticleDemo::tick(
  const Camera &camera,
  const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.updateBuffer(
    mCircleBuffer, 0, sizeof(glm::vec2) * mCircles.size(), &mCircles[0]);

  ComputePushConstant pushConstant = {
    glm::vec4(1.8f, 0.9f, 2.85f, 1.0f) * 5.0f,
    (int)mCircles.size(),
    (int)PARTICLE_COUNT,
    tick.dt
  };

  commandBuffer.bindComputePipeline(mCompute.res);
  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);
  commandBuffer.bindComputeUniforms(mParticleUniform, mCircleUniform);

  int groupCount = ((PARTICLE_COUNT) / 256) + 1;
  commandBuffer.dispatch(groupCount);
}

}
