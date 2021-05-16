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
  const WindowContextInfo &surfaceInfo) {
  GLFWwindow *window = (GLFWwindow *)surfaceInfo.handle;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  VulkanRenderPassConfig renderPassConfig(1, 1);
  renderPassConfig.addAttachment(
    LoadAndStoreOp::LoadThenStore, LoadAndStoreOp::DontCareThenDontCare,
    OutputUsage::Present, AttachmentType::Color, swapchain.mFormat);

  renderPassConfig.addSubpass(
    makeArray<uint32_t, AllocationType::Linear>(0U),
    makeArray<uint32_t, AllocationType::Linear>(),
    false);

  mImguiRenderPass.init(device, renderPassConfig);

  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance.mInstance;
  init_info.PhysicalDevice = device.mPhysicalDevice;
  init_info.Device = device.mLogicalDevice;
  init_info.QueueFamily = device.mQueueFamilies.graphicsFamily;
  init_info.Queue = device.mGraphicsQueue.mQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptorPool.mDescriptorPool;
  init_info.Allocator = NULL;
  init_info.MinImageCount = swapchain.mImages.size;
  init_info.ImageCount = swapchain.mImages.size;
  init_info.CheckVkResultFn = imguiCallback;
  ImGui_ImplVulkan_Init(&init_info, mImguiRenderPass.mRenderPass);

  ImGui::StyleColorsDark();

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

void VulkanImgui::imguiCallback(VkResult result) {

}

void VulkanImgui::render(const VulkanFrame &frame) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // General stuff
  ImGui::Begin("General");
  ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);

  ImGui::End();

  ImGui::Render();

  ImGui_ImplVulkan_RenderDrawData(
    ImGui::GetDrawData(),
    frame.primaryCommandBuffer.mCommandBuffer);
}

}
