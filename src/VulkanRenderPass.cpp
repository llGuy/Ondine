#include <assert.h>
#include "Log.hpp"
#include "Vulkan.hpp"
#include "VulkanSync.hpp"
#include "VulkanDevice.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine::Graphics {

VulkanRenderPassConfig::VulkanRenderPassConfig(
  uint32_t attachmentCount,
  uint32_t subpassCount)
  : mCreateInfo{},
    mAttachments(attachmentCount),
    mClearValues(attachmentCount),
    mAttachmentTypes(attachmentCount),
    mOutputUsages(attachmentCount),
    mSubpasses(subpassCount),
    mRefs(subpassCount * attachmentCount),
    mDeps(subpassCount + 1),
    mDepthIdx(-1) {
  mDeps.size = subpassCount + 1;
  mClearValues.zero();
  mRefs.zero();
  mDeps.zero();
}

void VulkanRenderPassConfig::addAttachment(
  LoadAndStoreOp loadAndStore, LoadAndStoreOp stencilLoadAndStore,
  OutputUsage outputUsage, AttachmentType type, VkFormat format) {
  assert(mAttachments.size < mAttachments.capacity);

  if (type == AttachmentType::Depth &&
      (stencilLoadAndStore == LoadAndStoreOp::ClearThenStore ||
       stencilLoadAndStore == LoadAndStoreOp::ClearThenDontCare)) {
    mDepthIdx = mAttachmentTypes.size;
    mClearValues[mClearValues.size++].depthStencil.depth = 1.0f;
  }
  else if (
    loadAndStore == LoadAndStoreOp::ClearThenStore ||
    loadAndStore == LoadAndStoreOp::ClearThenDontCare) {
    // Keep at 0
    ++mClearValues.size;
  }

  mAttachmentTypes[mAttachmentTypes.size++] = type;
  mOutputUsages[mOutputUsages.size++] = outputUsage;
  VkAttachmentDescription &desc = mAttachments[mAttachments.size++];

  VkImageLayout finalLayout;
  switch (outputUsage) {
  case OutputUsage::Present: {
    finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  } break;

  case OutputUsage::FragmentShaderRead:
  case OutputUsage::VertexShaderRead: {
    finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  } break;

  case OutputUsage::None: {
    if (type == AttachmentType::Color)
      finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    else if (type == AttachmentType::Depth)
      finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
    VK_ATTACHMENT_LOAD_OP_LOAD,
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

  if (loadOpIdx == 0) {
    if (type == AttachmentType::Color) {
      desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else if (type == AttachmentType::Depth) {
      desc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }
  }
  else {
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  }
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
  desc.pInputAttachments = inputsPtr;
  desc.pDepthStencilAttachment = depth;
}

void VulkanRenderPassConfig::finishConfiguration() {
  // Only if finishConfiguration hasn't been called yet
  if (mCreateInfo.sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO) {
    // Create first dependency
    VkSubpassDependency *firstDep = &mDeps[0];
    firstDep->srcSubpass = VK_SUBPASS_EXTERNAL;
    firstDep->dstSubpass = 0;
    firstDep->srcStageMask = computeMostRecentStage(mSubpasses[0]);
    firstDep->srcAccessMask = findAccessFlagsForStage(firstDep->srcStageMask);
    firstDep->dstStageMask = computeSubpassStage(mSubpasses[0]);
    firstDep->dstAccessMask = findAccessFlagsForStage(firstDep->dstStageMask);

    for (int i = 0; i < (int)mDeps.size - 2; ++i) {
      VkSubpassDependency *dep = &mDeps[i + 1];

      VkSubpassDescription *prev = &mSubpasses[i];
      VkSubpassDescription *next = &mSubpasses[i + 1];

      dep->srcSubpass = i;
      dep->dstSubpass = i + 1;

      if (prev->colorAttachmentCount > 0)
        dep->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      else
        dep->srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

      if (next->pDepthStencilAttachment)
        dep->dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      else
        dep->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

      dep->srcAccessMask = findAccessFlagsForStage(dep->srcStageMask);
      dep->dstAccessMask = findAccessFlagsForStage(dep->dstStageMask);
    }

    VkSubpassDependency *lastDep = &mDeps[mDeps.size - 1];
    VkSubpassDescription *lastSubpass = &mSubpasses[mSubpasses.size - 1];
    lastDep->srcSubpass = mSubpasses.size - 1;
    lastDep->dstSubpass = VK_SUBPASS_EXTERNAL;
    lastDep->srcStageMask = computeSubpassStage(*lastSubpass);
    lastDep->srcAccessMask = findAccessFlagsForStage(lastDep->srcStageMask);
    lastDep->dstStageMask = computeEarliestStage(*lastSubpass);
    lastDep->dstAccessMask = findAccessFlagsForStage(lastDep->dstStageMask);

    mCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    mCreateInfo.attachmentCount = mAttachments.size;
    mCreateInfo.pAttachments = mAttachments.data;
    mCreateInfo.subpassCount = mSubpasses.size;
    mCreateInfo.pSubpasses = mSubpasses.data;
    mCreateInfo.dependencyCount = mDeps.size;
    mCreateInfo.pDependencies = mDeps.data;
  }
}

enum class OrderedStage {
  LateFragmentTests,
  ColorAttachment,
  VertexShader,
  FragmentShader
};

static const VkPipelineStageFlagBits ORDERED_STAGES[] {
  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
};

VkPipelineStageFlagBits VulkanRenderPassConfig::computeMostRecentStage(
  const VkSubpassDescription &subpass) {
  int mostRecentStage = 0;

  for (int i = 0; i < subpass.colorAttachmentCount; ++i) {
    uint32_t idx = subpass.pColorAttachments[i].attachment;
    /*
      If this attachment is marked as ShaderRead, that means that the most
      recent access of this image was in the fragment shader (at some point
      earlier in the command buffer)
     */
    if (mOutputUsages[idx] == OutputUsage::FragmentShaderRead) {
      /* Because this is the latest, we can just return */
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (mOutputUsages[idx] == OutputUsage::VertexShaderRead) {
      mostRecentStage = MAX(mostRecentStage, (int)OrderedStage::VertexShader);
    }
    /*
      If this attachment is marked as anything else, that means that the most
      recent access of this image was by this same render pass when it was last
      written to. This is most likely going to be for things like presenting.
     */
    else {
      mostRecentStage = MAX(mostRecentStage, (int)OrderedStage::ColorAttachment);
    }
  }

  if (subpass.pDepthStencilAttachment) {
    if (mOutputUsages[mDepthIdx] == OutputUsage::FragmentShaderRead) {
      return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (mOutputUsages[mDepthIdx] == OutputUsage::VertexShaderRead) {
      mostRecentStage = MAX(mostRecentStage, (int)OrderedStage::VertexShader);
    }
    else {
      mostRecentStage = MAX(
        mostRecentStage,
        (int)OrderedStage::LateFragmentTests);
    }
  }

  return ORDERED_STAGES[mostRecentStage];
}

VkPipelineStageFlagBits VulkanRenderPassConfig::computeEarliestStage(
  const VkSubpassDescription &subpass) {
  int earliestStage = (int)OrderedStage::FragmentShader;

  for (int i = 0; i < subpass.colorAttachmentCount; ++i) {
    uint32_t idx = subpass.pColorAttachments[i].attachment;
    if (mOutputUsages[idx] == OutputUsage::VertexShaderRead) {
      return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    else if (mOutputUsages[idx] == OutputUsage::FragmentShaderRead) {
      earliestStage = MIN(earliestStage, (int)OrderedStage::FragmentShader);
    }
  }

  if (subpass.pDepthStencilAttachment) {
    if (mOutputUsages[mDepthIdx] == OutputUsage::VertexShaderRead) {
      return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    else if (mOutputUsages[mDepthIdx] == OutputUsage::FragmentShaderRead) {
      earliestStage = MIN(earliestStage, (int)OrderedStage::FragmentShader);
    }
  }

  return ORDERED_STAGES[earliestStage];
}

VkPipelineStageFlagBits VulkanRenderPassConfig::computeSubpassStage(
  const VkSubpassDescription &subpass) {
  if (subpass.pDepthStencilAttachment) {
    return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  }

  if (subpass.colorAttachmentCount > 0) {
    return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }

  LOG_ERROR("Subpass contains no color outputs nor depth output!\n");
  PANIC_AND_EXIT();

  return VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
}

VulkanRenderPass::VulkanRenderPass()
  : mRenderPass(VK_NULL_HANDLE) {
  
}

void VulkanRenderPass::init(
  const VulkanDevice &device,
  VulkanRenderPassConfig &config) {
  config.finishConfiguration();

  mClearValues.init(config.mClearValues.size);
  memcpy(
    mClearValues.data, config.mClearValues.data,
    config.mClearValues.size * sizeof(VkClearValue));
  mClearValues.size = config.mClearValues.size;

  VK_CHECK(
    vkCreateRenderPass(
      device.mLogicalDevice,
      &config.mCreateInfo,
      NULL,
      &mRenderPass));

  mAttachments = config.mAttachments;
  mAttachmentTypes = config.mAttachmentTypes;
  mSubpasses = config.mSubpasses;
  mRefs = config.mRefs;
}

void VulkanRenderPass::destroy(const VulkanDevice &device) {
  vkDestroyRenderPass(device.mLogicalDevice, mRenderPass, nullptr);
  mRenderPass = VK_NULL_HANDLE;

  mAttachments.free();
  mClearValues.free();
  mAttachmentTypes.free();
  mSubpasses.free();
  mRefs.free();
}

}
