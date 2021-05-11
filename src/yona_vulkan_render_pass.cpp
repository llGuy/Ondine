#include <assert.h>
#include "yona_log.hpp"
#include "yona_vulkan_render_pass.hpp"

namespace Yona {

void VulkanRenderPassConfig::beginConfiguration(
  uint32_t attachmentCount,
  uint32_t subpassCount) {
  mAttachments.init(attachmentCount);
  mAttachmentTypes.init(attachmentCount);
  mSubpasses.init(subpassCount);
  mRefs.init(subpassCount * attachmentCount);
  mDeps.init(subpassCount + 1);
  mDepthIdx = -1;
}

void VulkanRenderPassConfig::addAttachment(
  LoadAndStoreOp loadAndStore, LoadAndStoreOp stencilLoadAndStore,
  OutputUsage outputUsage, AttachmentType type, VkFormat format) {
  assert(mAttachments.size < mAttachments.capacity);

  if (type == AttachmentType::Depth) {
    mDepthIdx = mAttachmentTypes.size;
  }

  mAttachmentTypes[mAttachmentTypes.size++] = type;
  VkAttachmentDescription &desc = mAttachments[mAttachments.size++];

  /* Image layouts */
  desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageLayout finalLayout;
  switch (outputUsage) {
  case OutputUsage::Present: {
    finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  } break;

  case OutputUsage::ShaderRead: {
    finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  } break;

  default: {
    LOG_ERRORV("Unhandled output usage for attachment: %d\n", (int)outputUsage);
    PANIC_AND_EXIT();
  } break;
  }

  desc.finalLayout = finalLayout;

  /* Doubt I will need more than 1 sample for this program */
  desc.samples = VK_SAMPLE_COUNT_1_BIT;
  desc.format = format;

  static const VkAttachmentLoadOp LOAD_OPS[] = {
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE
  };

  static const VkAttachmentStoreOp STORE_OPS[] = {
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE
  };

  uint32_t loadOpIdx = (uint32_t)loadAndStore & 0b11;
  uint32_t storeOpIdx = ((uint32_t)(loadAndStore) >> 2) & 0b11;

  uint32_t stencilLoadOpIdx = (uint32_t)stencilLoadAndStore & 0b11;
  uint32_t stencilStoreOpIdx = ((uint32_t)(stencilLoadAndStore) >> 2) & 0b11;

  desc.loadOp = LOAD_OPS[loadOpIdx];
  desc.storeOp = STORE_OPS[storeOpIdx];
  desc.stencilLoadOp = LOAD_OPS[stencilLoadOpIdx];
  desc.stencilStoreOp = STORE_OPS[stencilStoreOpIdx];
}

void VulkanRenderPassConfig::addSubpass(
  const Array<uint32_t, AllocationType::Linear> &colors,
  const Array<uint32_t, AllocationType::Linear> &inputs,
  bool hasDepth) {
  VkSubpassDescription &desc = mSubpasses[mSubpasses.size++];

  VkAttachmentReference *start = &mRefs[mRefs.size];
  uint32_t refCount = 0;

  for (int i = 0; i < colors.size; ++i) {
    uint32_t colorIdx = colors[i];

    VkAttachmentReference *current = &start[refCount];
    current->attachment = colorIdx;
    current->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    ++refCount;
  }

  VkAttachmentReference *inputsPtr = (inputs.size > 0) ?
    &start[refCount] : nullptr;

  for (int i = 0; i < inputs.size; ++i) {
    uint32_t inputIdx = inputs[i];

    VkAttachmentReference *current = &start[refCount];
    current->attachment = inputIdx;
    current->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ++refCount;
  }

  VkAttachmentReference *depth = nullptr;

  if (hasDepth) {
    assert(mDepthIdx >= 0);

    depth = &start[refCount];
    depth->attachment = mDepthIdx;
    depth->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    ++refCount;
  }

  mRefs.size += refCount;

  desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  desc.colorAttachmentCount = colors.size;
  desc.pColorAttachments = start;
  desc.inputAttachmentCount = inputs.size;
  desc.pInputAttachments = start;
  desc.pDepthStencilAttachment = depth;
}

void VulkanRenderPassConfig::endConfiguration() {
  /* Set dependencies */
  for (int i = 0; i < mDeps.size; ++i) {
    
  }
}

}
