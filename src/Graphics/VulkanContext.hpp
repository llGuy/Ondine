#pragma once

#include "IO.hpp"
#include "VulkanSync.hpp"
#include "VulkanImgui.hpp"
#include "VulkanFrame.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSurface.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanCommandPool.hpp"

namespace Ondine::Graphics {

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
  void initContext(const Core::WindowContextInfo &surfaceInfo);
  void initImgui(
    const Core::WindowContextInfo &surfaceInfo,
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
