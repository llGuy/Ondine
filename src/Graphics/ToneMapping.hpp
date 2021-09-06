#pragma once

#include "Tick.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "TrackedResource.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

struct ToneMappingProperties {
  glm::vec4 white;
  float exposure;
};

// TODO: Support auto-eye adaption
class ToneMapping :
  public RenderStage,
  public ResourceTracker {
public:
  ToneMapping() = default;
  ~ToneMapping() override = default;

  void init(
    VulkanContext &graphicsContext,
    VkExtent2D initialExtent,
    const ToneMappingProperties &initialProperties);

  void render(VulkanFrame &frame, const VulkanUniform &previousOutput);
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  VkExtent2D mExtent;
  VulkanTexture mTexture;
  VulkanFramebuffer mFBO;
  VulkanRenderPass mRenderPass;
  VulkanUniform mToneMappingOutput;
  ToneMappingProperties mProperties;
  TrackedResource<VulkanPipeline, ToneMapping> mPipeline;
};

}
