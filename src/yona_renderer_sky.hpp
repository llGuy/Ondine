#pragma once

#include "yona_sky_def.hpp"
#include "yona_vulkan_frame.hpp"
#include "yona_vulkan_buffer.hpp"
#include "yona_vulkan_uniform.hpp"
#include "yona_vulkan_pipeline.hpp"
#include "yona_vulkan_render_pass.hpp"
#include "yona_vulkan_framebuffer.hpp"

namespace Yona {

class VulkanContext;

class RendererSky {
public:
  void init(VulkanContext &graphicsContext);

  // For testing
  void tick(const VulkanFrame &frame);

private:
  void initSkyProperties(VulkanContext &graphicsContext);

  void preparePrecompute(VulkanContext &graphicsContext);

  void prepareTransmittancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void precomputeTransmittance(
    const VulkanCommandBuffer &commandBuffer);

  void precompute(VulkanContext &graphicsContext);

private:
  static constexpr size_t TRANSMITTANCE_WIDTH = 256;
  static constexpr size_t TRANSMITTANCE_HEIGHT = 64;

  SkyProperties mSkyProperties;
  VulkanBuffer mSkyPropertiesBuffer;
  VulkanUniform mSkyPropertiesUniform;

  VulkanPipeline mPrecomputeTransmittancePipeline;
  VulkanRenderPass mPrecomputeTransmittanceRenderPass;
  VulkanFramebuffer mPrecomputedTransmittanceFBO;
  VulkanTexture mPrecomputedTransmittance;
};

}
