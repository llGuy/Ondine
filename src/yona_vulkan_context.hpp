#pragma once

#include "yona_io.hpp"
#include "yona_vulkan_sync.hpp"
#include "yona_vulkan_imgui.hpp"
#include "yona_vulkan_frame.hpp"
#include "yona_vulkan_device.hpp"
#include "yona_vulkan_surface.hpp"
#include "yona_vulkan_instance.hpp"
#include "yona_vulkan_swapchain.hpp"
#include "yona_vulkan_descriptor.hpp"
#include "yona_vulkan_command_pool.hpp"

namespace Yona {

struct WindowContextInfo;

struct VulkanContextProperties {
  VkFormat depthFormat;
  VkFormat swapchainFormat;
  VkExtent2D swapchainExtent;
};

class VulkanContext {
public:
  VulkanContext();

  void initInstance();
  void initContext(const WindowContextInfo &surfaceInfo);
  void initImgui(
    const WindowContextInfo &surfaceInfo,
    const VulkanRenderPass &renderPass);

  /* Acquires the next swapchain image */
  VulkanFrame beginFrame();
  /* Presents to the screen */
  void endFrame(const VulkanFrame &frame);

  /* Starts the render pass which renders to the swapchain */
  void beginSwapchainRender(const VulkanFrame &frame);
  /* Ends the render pass which renders to the swapchain */
  void endSwapchainRender(const VulkanFrame &frame);

  void resize(const Resolution &newResolution);

  void skipFrame();

  const VulkanDevice &device() const;
  VulkanDescriptorSetLayoutMaker &descriptorLayouts();
  const VulkanCommandPool &commandPool() const;
  const VulkanDescriptorPool &descriptorPool() const;
  const VulkanRenderPass &finalRenderPass() const;
  VulkanContextProperties getProperties() const;
  const VulkanImgui &imgui() const;

private:
  VulkanInstance mInstance;
  VulkanSurface mSurface;
  VulkanDevice mDevice;
  VulkanSwapchain mSwapchain;
  VulkanRenderPass mFinalRenderPass;
  Array<VulkanFramebuffer> mFinalFramebuffers;
  VulkanCommandPool mCommandPool;
  Array<VulkanCommandBuffer> mPrimaryCommandBuffers;
  Array<VulkanSemaphore> mImageReadySemaphores;
  Array<VulkanSemaphore> mRenderFinishedSemaphores;
  Array<VulkanFence> mFences;
  uint32_t mFramesInFlight;
  uint32_t mCurrentFrame;
  VulkanDescriptorPool mDescriptorPool;
  VulkanDescriptorSetLayoutMaker mDescriptorSetLayouts;
  VulkanImgui mImgui;
  bool mSkipFrame;
};

}
