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
  void tick(VulkanFrame &frame);

private:
  void initSkyProperties(VulkanContext &graphicsContext);
  void initTemporaryPrecomputeTextures(VulkanContext &graphicsContext);

  void preparePrecompute(VulkanContext &graphicsContext);

  void prepareTransmittancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void prepareSingleScatteringPrecompute(
    const Buffer &precomputeVsh,
    const Buffer &precomputeGsh,
    VulkanContext &graphicsContext);

  void precomputeTransmittance(
    VulkanCommandBuffer &commandBuffer);

  void precomputeSingleScattering(
    VulkanCommandBuffer &commandBuffer);

  void precompute(VulkanContext &graphicsContext);

private:
  static constexpr size_t TRANSMITTANCE_WIDTH = 256;
  static constexpr size_t TRANSMITTANCE_HEIGHT = 64;
  static constexpr size_t SCATTERING_TEXTURE_R_SIZE = 32;
  static constexpr size_t SCATTERING_TEXTURE_MU_SIZE = 128;
  static constexpr size_t SCATTERING_TEXTURE_MU_S_SIZE = 32;
  static constexpr size_t SCATTERING_TEXTURE_NU_SIZE = 8;
  static constexpr size_t SCATTERING_TEXTURE_WIDTH =
    SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
  static constexpr size_t SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
  static constexpr size_t SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;
  static constexpr size_t IRRADIANCE_TEXTURE_WIDTH = 64;
  static constexpr size_t IRRADIANCE_TEXTURE_HEIGHT = 16;
  static constexpr VkFormat PRECOMPUTED_TEXTURE_FORMAT =
    VK_FORMAT_R16G16B16A16_SFLOAT;

  SkyProperties mSkyProperties;
  VulkanBuffer mSkyPropertiesBuffer;
  VulkanUniform mSkyPropertiesUniform;

  VulkanPipeline mPrecomputeTransmittancePipeline;
  VulkanRenderPass mPrecomputeTransmittanceRenderPass;
  VulkanFramebuffer mPrecomputeTransmittanceFBO;
  VulkanTexture mPrecomputedTransmittance;
  VulkanUniform mPrecomputedTransmittanceUniform;

  struct SingleScatteringPushConstant {
    int layer;
  };

  VulkanPipeline mPrecomputeSingleScatteringPipeline;
  VulkanRenderPass mPrecomputeSingleScatteringRenderPass;
  VulkanFramebuffer mPrecomputeSingleScatteringFBO;
  VulkanTexture mPrecomputedSingleScattering;

  /* Temporary textures */
  VulkanTexture mDeltaRayleighScatteringTexture;
  VulkanTexture mDeltaMieScatteringTexture;
};

}
