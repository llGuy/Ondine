#include <imgui.h>
#include "Buffer.hpp"
#include "Window.hpp"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "VulkanImgui.hpp"
#include "VulkanFrame.hpp"
#include "RendererDebug.hpp"
#include "VulkanInstance.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanCommandPool.hpp"

namespace Ondine::Graphics {

void VulkanImgui::init(
  const VulkanInstance &instance,
  const VulkanDevice &device,
  const VulkanSwapchain &swapchain,
  const VulkanDescriptorPool &descriptorPool,
  const VulkanCommandPool &commandPool,
  const Core::WindowContextInfo &surfaceInfo,
  const VulkanRenderPass &renderPass) {
  GLFWwindow *window = (GLFWwindow *)surfaceInfo.handle;
  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = instance.mInstance;
  initInfo.PhysicalDevice = device.mPhysicalDevice;
  initInfo.Device = device.mLogicalDevice;
  initInfo.QueueFamily = device.mQueueFamilies.graphicsFamily;
  initInfo.Queue = device.mGraphicsQueue.mQueue;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = descriptorPool.mDescriptorPool;
  initInfo.Allocator = NULL;
  initInfo.MinImageCount = swapchain.mImages.size;
  initInfo.ImageCount = swapchain.mImages.size;
  initInfo.CheckVkResultFn = imguiCallback;
  ImGui_ImplVulkan_Init(&initInfo, renderPass.mRenderPass);

  VulkanCommandBuffer commandBuffer = commandPool.makeCommandBuffer(
    device, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer.mCommandBuffer);
  commandBuffer.end();

  device.mGraphicsQueue.submitCommandBuffer(
    commandBuffer,
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    makeArray<VulkanSemaphore, AllocationType::Linear>(),
    0, VulkanFence());

  device.mGraphicsQueue.idle();
}

void VulkanImgui::beginRender() const {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void VulkanImgui::endRender(const VulkanFrame &frame) const {
  frame.primaryCommandBuffer.dbgInsertMarker("ImGui", Graphics::DBG_IMGUI_COLOR);

  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData(
    ImGui::GetDrawData(),
    frame.primaryCommandBuffer.mCommandBuffer);
}

void VulkanImgui::imguiCallback(VkResult result) {
  
}

}
