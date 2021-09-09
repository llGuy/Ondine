#pragma once

#include "Tick.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "TrackedResource.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::View {

class EditorView;

}

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

  void render(VulkanFrame &frame, const RenderStage &previousOutput);
  void resize(VulkanContext &vulkanContext, Resolution newResolution);

  const VulkanRenderPass &renderPass() const override;
  const VulkanFramebuffer &framebuffer() const override;
  const VulkanUniform &uniform() const override;
  VkExtent2D extent() const override;

private:
  void initTargets(VulkanContext &graphicsContext);
  void destroyTargets(VulkanContext &graphicsContext);

private:
  static const char *const TONE_MAPPING_FRAG_SPV;
  static constexpr VkFormat TONE_MAPPING_TEXTURE_FORMAT =
    VK_FORMAT_R8G8B8A8_UNORM;

  VkExtent2D mExtent;
  VulkanTexture mTexture;
  VulkanFramebuffer mFBO;
  VulkanRenderPass mRenderPass;
  VulkanUniform mToneMappingOutput;
  ToneMappingProperties mProperties;
  TrackedResource<VulkanPipeline, ToneMapping> mPipeline;

  friend class View::EditorView;
};

}
