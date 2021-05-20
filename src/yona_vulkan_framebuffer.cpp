#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_vulkan.hpp"
#include "yona_vulkan_framebuffer.hpp"

namespace Yona {

VulkanFramebufferConfig::VulkanFramebufferConfig(
  size_t attachmentCount,
  const VulkanRenderPass &renderPass)
  : mCreateInfo{},
    mResolution{0, 0},
    mAttachments(attachmentCount),
    mCompatibleRenderPass(renderPass) {

}

void VulkanFramebufferConfig::addAttachment(const VulkanTexture &texture) {
  // Uninitialised
  if (mResolution.width == 0) {
    mResolution = {texture.mExtent.width, texture.mExtent.height};
    mLayerCount = texture.mViewLayerCount;
  }
  else if (
    mResolution.width != texture.mExtent.width ||
    mResolution.height != texture.mExtent.height) {
    LOG_ERROR("Inconsistent framebuffer attachment resolutions!\n");
    PANIC_AND_EXIT();
  }

  mAttachments[mAttachments.size++] = texture;
}

void VulkanFramebufferConfig::finishConfiguration() {
  if (mCreateInfo.sType != VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO) {
    mCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    mCreateInfo.renderPass = mCompatibleRenderPass.mRenderPass;
    mCreateInfo.attachmentCount = mAttachments.size;
    mCreateInfo.width = mResolution.width;
    mCreateInfo.height = mResolution.height;
    mCreateInfo.layers = mLayerCount;

    Array<VkImageView, AllocationType::Linear> views(mAttachments.size);
    for (int i = 0; i < mAttachments.size; ++i) {
      views[i] = mAttachments[i].mImageView;
    }

    mCreateInfo.pAttachments = views.data;
  }
}

void VulkanFramebuffer::init(
  const VulkanDevice &device,
  VulkanFramebufferConfig &config) {
  config.finishConfiguration();

  VK_CHECK(
    vkCreateFramebuffer(
      device.mLogicalDevice,
      &config.mCreateInfo,
      NULL,
      &mFramebuffer));
}

}
