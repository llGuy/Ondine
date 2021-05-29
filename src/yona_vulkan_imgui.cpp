#include <imgui.h>
#include "yona_buffer.hpp"
#include "yona_window.hpp"
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "yona_vulkan_imgui.hpp"
#include "yona_vulkan_frame.hpp"
#include "yona_vulkan_instance.hpp"
#include "yona_vulkan_swapchain.hpp"
#include "yona_vulkan_descriptor.hpp"
#include "yona_vulkan_render_pass.hpp"
#include "yona_vulkan_command_pool.hpp"

namespace Yona {

void VulkanImgui::init(
  const VulkanInstance &instance,
  const VulkanDevice &device,
  const VulkanSwapchain &swapchain,
  const VulkanDescriptorPool &descriptorPool,
  const VulkanCommandPool &commandPool,
  const WindowContextInfo &surfaceInfo,
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
  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData(
    ImGui::GetDrawData(),
    frame.primaryCommandBuffer.mCommandBuffer);
}

void VulkanImgui::imguiCallback(VkResult result) {
  
}

}
