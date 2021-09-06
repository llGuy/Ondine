#pragma once

#include "Delegate.hpp"
#include <unordered_map>
#include "FileSystem.hpp"
#include "RenderStage.hpp"
#include "VulkanContext.hpp"
#include "TrackedResource.hpp"

namespace Ondine::View {

class EditorView;

}

namespace Ondine::Graphics {

class Pixelater :
  public RenderStage,
  public ResourceTracker {
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
  static const char *const PIXELATER_FRAG_SPV;

  struct PushConstant {
    float pixelationStrength;
    float width;
    float height;
  };

  VulkanUniform mOutput;

  // Needs to be updated when shaders get written to
  TrackedResource<VulkanPipeline, Pixelater> mPipeline;

  std::unordered_map<
    Core::TrackPathID, TrackedResourceInterface *> mResourceRefs;

  VulkanRenderPass mRenderPass;
  VulkanFramebuffer mFBO;
  VulkanTexture mTexture;
  VkExtent2D mExtent;

  // Will only be relevant in DEV builds
  friend class View::EditorView;
};

}
