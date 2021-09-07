#pragma once

#include "RenderStage.hpp"
#include "VulkanUniform.hpp"
#include "TrackedResource.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

struct BloomProperties {
  glm::vec4 bloomIntensity;
};

class BloomRenderer :
  public RenderStage,
  public ResourceTracker {
public:
  BloomRenderer() = default;
  ~BloomRenderer() override = default;

  void init(
    VulkanContext &graphicsContext,
    VkExtent2D initialExtent);
  void render(VulkanFrame &frame, const VulkanUniform &previous);
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  static const char *BLOOM_PREFILTER_FRAG_SPV;
  static const char *BLOOM_BLUR_FRAG_SPV;
  static constexpr VkFormat BLOOM_TEXTURE_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

  struct BlurTexturePair {
    /* Horizontal and verical blur targets */
    VulkanTexture targets[2];
    VulkanUniform uniforms[2];
    VulkanFramebuffer fbos[2];

    /* TODO: Reuse target textures but for debugging purposes, do this */
    VulkanTexture addition;
    VulkanUniform additionUniform;
    VulkanFramebuffer additionFBO;
  };

  VkExtent2D mExtent;
  VulkanRenderPass mRenderPass;

  VulkanTexture mPrefiltered;
  VulkanUniform mPrefilteredUniform;
  VulkanFramebuffer mPrefilteredFBO;
  Array<BlurTexturePair> mBlurTargets;

  TrackedResource<VulkanPipeline, BloomRenderer> mBlurPipeline;
  TrackedResource<VulkanPipeline, BloomRenderer> mPrefilterPipeline;
};

}
