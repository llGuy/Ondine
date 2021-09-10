#pragma once

#include "RenderStage.hpp"
#include "VulkanUniform.hpp"
#include "TrackedResource.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::View {

class EditorView;

}

namespace Ondine::Graphics {

struct BloomProperties {
  glm::vec4 intensity;
  glm::vec4 scale;
  float threshold;
  bool horizontal;
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
  void render(VulkanFrame &frame, const RenderStage &previous);
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  struct Target {
    VulkanTexture texture;
    VulkanUniform uniform;
    VulkanFramebuffer fbo;
  };

  void initTargets(VulkanContext &graphicsContext);
  void initTarget(Target &target, VkExtent2D extent, VulkanContext &context);
  void destroyTarget(Target &target, VulkanContext &context);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  static const char *BLOOM_PREFILTER_FRAG_SPV;
  static const char *BLOOM_BLUR_FRAG_SPV;
  static const char *BLOOM_ADDITIVE_FRAG_SPV;
  static constexpr VkFormat BLOOM_TEXTURE_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

  struct BlurTexturePair {
    VkExtent2D extent;

    /* Horizontal and verical blur targets */
    Target blurTargets[2];

    /* TODO: Reuse target textures but for debugging purposes, do this */
    Target additiveTarget;
  };

  VkExtent2D mExtent;
  VulkanRenderPass mRenderPass;

  VkExtent2D mPrefilteredExtent;
  Target mPrefilteredTarget;

  Array<BlurTexturePair> mBlurTargets;

  TrackedResource<VulkanPipeline, BloomRenderer> mBlurPipeline;
  TrackedResource<VulkanPipeline, BloomRenderer> mPrefilterPipeline;
  TrackedResource<VulkanPipeline, BloomRenderer> mAdditivePipeline;

  BloomProperties mProperties;

  friend class View::EditorView;
};

}
