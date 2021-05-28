#include "yona_vulkan_context.hpp"

#ifndef NDEBUG
constexpr bool ENABLE_VALIDATION = true;
#else
constexpr bool ENABLE_VALIDATION = false;
#endif

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

namespace Yona {

VulkanContext::VulkanContext()
  : mInstance(ENABLE_VALIDATION),
    mFramesInFlight(FRAMES_IN_FLIGHT),
    mSkipFrame(false),
    mCurrentFrame(0) {
  
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
  requiredFeatures.features.independentBlend = VK_TRUE;
  mDevice.init(DeviceType::DiscreteGPU, mInstance, mSurface, requiredFeatures);

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

VulkanFrame VulkanContext::beginFrame() {
  if (mSkipFrame) {
    VulkanFrame frame {
      mDevice,
      mPrimaryCommandBuffers[0],
      0,
      0,
      {mSwapchain.mExtent.width, mSwapchain.mExtent.height},
      true
    };

    mSkipFrame = false;

    return frame;
  }
  else {
    uint32_t imageIndex = mSwapchain.acquireNextImage(
      mDevice, mImageReadySemaphores[mCurrentFrame]);

    mFences[mCurrentFrame].wait(mDevice);
    mFences[mCurrentFrame].reset(mDevice);

    // Begin primary command buffer
    VulkanCommandBuffer &currentCommandBuffer =
      mPrimaryCommandBuffers[imageIndex];

    currentCommandBuffer.begin(0, nullptr);

    mImgui.beginRender();

    VulkanFrame frame {
      mDevice,
      currentCommandBuffer,
      imageIndex,
      mCurrentFrame,
      {mSwapchain.mExtent.width, mSwapchain.mExtent.height},
      false
    };

    return frame;
  }
}

void VulkanContext::endFrame(const VulkanFrame &frame) {
  frame.primaryCommandBuffer.end();

  const VulkanSemaphore &imageReady = mImageReadySemaphores[mCurrentFrame];
  const VulkanSemaphore &renderDone = mRenderFinishedSemaphores[mCurrentFrame];

  mDevice.mGraphicsQueue.submitCommandBuffer(
    frame.primaryCommandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(imageReady),
    makeArray<VulkanSemaphore, AllocationType::Linear>(renderDone),
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    mFences[mCurrentFrame]);

  VkResult result = mDevice.mPresentQueue.present(mSwapchain, renderDone);

  if (result != VK_SUCCESS) {
    // Need to handle this
  }

  mCurrentFrame = (mCurrentFrame + 1) % mFramesInFlight;
}

void VulkanContext::beginSwapchainRender(const VulkanFrame &frame) {
  frame.primaryCommandBuffer.beginRenderPass(
    mFinalRenderPass,
    mFinalFramebuffers[frame.imageIndex],
    {0, 0},
    mSwapchain.mExtent);
}

void VulkanContext::endSwapchainRender(const VulkanFrame &frame) {
  mImgui.endRender(frame);

  frame.primaryCommandBuffer.endRenderPass();
}

void VulkanContext::resize(const Resolution &newResolution) {
  mDevice.idle();

  mSwapchain.destroy(mDevice);
  mFinalRenderPass.destroy(mDevice);

  for (int i = 0; i < mFinalFramebuffers.size; ++i) {
    mFinalFramebuffers[i].destroy(mDevice);
  }

  mFinalFramebuffers.free();

  mDevice.updateSurfaceCapabilities(mSurface);

  mSwapchain.init(mDevice, mSurface, newResolution);

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

  mFinalFramebuffers = mSwapchain.makeFramebuffers(mDevice, mFinalRenderPass);
}

void VulkanContext::skipFrame() {
  mSkipFrame = true;
}

const VulkanDevice &VulkanContext::device() const {
  return mDevice;
}

VulkanDescriptorSetLayoutMaker &VulkanContext::descriptorLayouts() {
  return mDescriptorSetLayouts;
}

const VulkanCommandPool &VulkanContext::commandPool() const {
  return mCommandPool;
}

const VulkanDescriptorPool &VulkanContext::descriptorPool() const {
  return mDescriptorPool;
}

const VulkanRenderPass &VulkanContext::finalRenderPass() const {
  return mFinalRenderPass;
}

VulkanContextProperties VulkanContext::getProperties() const {
  VulkanContextProperties properties = {};
  properties.swapchainFormat = mSwapchain.mFormat;
  properties.swapchainExtent = mSwapchain.mExtent;
  properties.depthFormat = mDevice.mDepthFormat;
  return properties;
}

}
