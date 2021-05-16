#pragma once

#include "yona_vulkan_pipeline.hpp"
#include "yona_vulkan_render_pass.hpp"
#include "yona_vulkan_framebuffer.hpp"

namespace Yona {

class VulkanContext;

class RendererSky {
public:
  void init(VulkanContext &graphicsContext);

private:
  void preparePrecompute(VulkanContext &graphicsContext);
  void prepareTransmittancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void precompute();

private:
  VulkanPipeline mPrecomputeTransmittancePipeline;
  VulkanRenderPass mPrecomputeTransmittanceRenderPass;
  VulkanFramebuffer mPrecomputedTransmittanceFBO;
  VulkanTexture mPrecomputedTransmittance;
};

}
