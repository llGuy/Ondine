#include "yona_vulkan_context.hpp"

#ifndef DEBUG
constexpr bool ENABLE_VALIDATION = true;
#else
constexpr bool ENABLE_VALIDATION = false;
#endif

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

namespace Yona {

VulkanContext::VulkanContext()
  : mInstance(ENABLE_VALIDATION),
    mFramesInFlight(FRAMES_IN_FLIGHT) {
  
}

void VulkanContext::initInstance() {
  /* Just instance */
  mInstance.init();
}

void VulkanContext::initContext(const WindowContextInfo &surfaceInfo) {
  // Surface
  mSurface.init(mInstance, surfaceInfo);

  // Device
  DeviceRequestedFeatures requiredFeatures = {};
  requiredFeatures.count = 1;
  requiredFeatures.features.geometryShader = VK_TRUE;
  mDevice.init(DeviceType::Any, mInstance, mSurface, requiredFeatures);

  // Swapchain
  mSwapchain.init(mDevice, mSurface, surfaceInfo.resolution);

  // Final render pass
  VulkanRenderPassConfig finalRenderPassConfig (1, 1);
  finalRenderPassConfig.addAttachment(
    LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
    OutputUsage::Present, AttachmentType::Color,
    mSwapchain.mFormat);
  finalRenderPassConfig.addSubpass(
    makeArray<uint32_t, AllocationType::Linear>(0U),
    makeArray<uint32_t, AllocationType::Linear>(),
    false);
  mFinalRenderPass.init(mDevice, finalRenderPassConfig);

  // Final framebuffers
  mFinalFramebuffers = mSwapchain.makeFramebuffers(mDevice, mFinalRenderPass);

  // Command pool
  mCommandPool.init(mDevice);

  // Primary command buffers
  mPrimaryCommandBuffers.init(mSwapchain.mImages.size);
  mCommandPool.makeCommandBuffers(
    mDevice,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    mPrimaryCommandBuffers);

  // Synchronisation primitives
  mImageReadySemaphores.init(mFramesInFlight);
  mRenderFinishedSemaphores.init(mFramesInFlight);
  mFences.init(mFramesInFlight);
  for (int i = 0; i < mFramesInFlight; ++i) {
    mImageReadySemaphores[i].init(mDevice);
    mRenderFinishedSemaphores[i].init(mDevice);
    mFences[i].init(mDevice, VK_FENCE_CREATE_SIGNALED_BIT);

    ++mImageReadySemaphores.size,
      ++mRenderFinishedSemaphores.size,
      ++mFences.size;
  }

  // Descriptor set pool
  mDescriptorPool.init(
    mDevice,
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_SAMPLER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC },
    VulkanDescriptorTypeInfo{ 1000, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

  mDescriptorSetLayouts.init();

  mImgui.init(
    mInstance, mDevice, mSwapchain, mDescriptorPool,
    mCommandPool, surfaceInfo);
}

void VulkanContext::beginSwapchainRender() {
  
}

void VulkanContext::endSwapchainRender() {
  
}

const VulkanDevice &VulkanContext::device() const {
  return mDevice;
}

VulkanContextProperties VulkanContext::getProperties() const {
  VulkanContextProperties properties = {};
  properties.swapchainFormat = mSwapchain.mFormat;
  properties.depthFormat = mDevice.mDepthFormat;
  return properties;
}

}
