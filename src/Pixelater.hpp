#pragma once

#include "RenderStage.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Graphics {

class Pixelater : public RenderStage {
public:
  Pixelater() = default;
  ~Pixelater() override = default;

  void init(VulkanContext &graphicsContext, const VkExtent2D &initialExtent);

  void render(VulkanFrame &frame, const RenderStage &prev);
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  // 1.0f = no pixelation - increase to increase pixelation effect
  void setPixelationStrength(float strength);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

public:
  float pixelationStrength;

private:
  // For resizing
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  struct PushConstant {
    float pixelationStrength;
    float width;
    float height;
  };

  VulkanUniform mOutput;
  VulkanPipeline mPipeline;
  VulkanRenderPass mRenderPass;
  VulkanFramebuffer mFBO;
  VulkanTexture mTexture;
  VkExtent2D mExtent;
};

}
