#include "Utils.hpp"
#include "Vulkan.hpp"
#include "VulkanQueue.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanCommandBuffer.hpp"

namespace Ondine {

void VulkanQueue::idle() const {
  vkQueueWaitIdle(mQueue);
}

void VulkanQueue::submitCommandBuffer(
  const VulkanCommandBuffer &commandBuffer,
  const Array<VulkanSemaphore, AllocationType::Linear> &waitSemaphores,
  const Array<VulkanSemaphore, AllocationType::Linear> &signalSemaphores,
  VkPipelineStageFlags waitStage,
  const VulkanFence &fence) const {
  VkSemaphore *waitsRaw = STACK_ALLOC(VkSemaphore, waitSemaphores.size);
  for (int i = 0; i < waitSemaphores.size; ++i) {
    waitsRaw[i] = waitSemaphores[i].mSemaphore;
  }

  VkSemaphore *signalsRaw = STACK_ALLOC(VkSemaphore, signalSemaphores.size);
  for (int i = 0; i < signalSemaphores.size; ++i) {
    signalsRaw[i] = signalSemaphores[i].mSemaphore;
  }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer.mCommandBuffer;
  submitInfo.waitSemaphoreCount = waitSemaphores.size;
  submitInfo.pWaitSemaphores = waitsRaw;
  submitInfo.pWaitDstStageMask = &waitStage;
  submitInfo.signalSemaphoreCount = signalSemaphores.size;
  submitInfo.pSignalSemaphores = signalsRaw;

  VK_CHECK(vkQueueSubmit(mQueue, 1, &submitInfo, fence.mFence));
}

VkResult VulkanQueue::present(
  const VulkanSwapchain &swapchain,
  const VulkanSemaphore &wait) const {
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &wait.mSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain.mSwapchain;
  presentInfo.pImageIndices = &swapchain.mImageIndex;

  return vkQueuePresentKHR(mQueue, &presentInfo);
}

}
