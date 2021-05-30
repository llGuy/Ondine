#pragma once

#include "Utils.hpp"
#include "Buffer.hpp"
#include <vulkan/vulkan.h>
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"

namespace Yona {

enum class LoadAndStoreOp {
  LoadThenStore        = 0 | (0 << 2),
  LoadThenDontCare     = 0 | (1 << 2),

  ClearThenStore       = 1 | (0 << 2),
  ClearThenDontCare    = 1 | (1 << 2),

  DontCareThenStore    = 2 | (0 << 2),
  DontCareThenDontCare = 2 | (1 << 2)
};

enum class SubpassConsequence {
  WritesToColor = BIT(0),
  WritesToDepth = BIT(1)
};

enum class OutputUsage {
  Present,
  VertexShaderRead, // Rare
  FragmentShaderRead,
  None
};

using AttachmentType = TextureContents;

/* 
   This is always going to be a temporary object.
   For more complex vulkan objects (like this one) this sort of paradigm
   will be followed (having a separate config class and passing to the object)
*/
class VulkanRenderPassConfig {
public:
  VulkanRenderPassConfig(uint32_t attachmentCount, uint32_t subpassCount);

  void addAttachment(
    LoadAndStoreOp loadAndStore, LoadAndStoreOp stencilLoadAndStore,
    OutputUsage outputUsage, AttachmentType type, VkFormat format);

  void addSubpass(
    const Array<uint32_t, AllocationType::Linear> &colors,
    const Array<uint32_t, AllocationType::Linear> &inputs,
    bool hasDepth);

private:
  /* Gets called by VulkanRenderPass in init */
  void finishConfiguration();

  VkPipelineStageFlagBits computeMostRecentStage(const VkSubpassDescription &);
  VkPipelineStageFlagBits computeEarliestStage(const VkSubpassDescription &);
  VkPipelineStageFlagBits computeSubpassStage(const VkSubpassDescription &);

private:
  Array<VkAttachmentDescription, AllocationType::Freelist> mAttachments;
  Array<VkClearValue, AllocationType::Linear> mClearValues;
  Array<AttachmentType, AllocationType::Freelist> mAttachmentTypes;
  Array<OutputUsage, AllocationType::Linear> mOutputUsages;
  Array<VkSubpassDescription, AllocationType::Freelist> mSubpasses;
  Array<VkAttachmentReference, AllocationType::Freelist> mRefs;
  Array<VkSubpassDependency, AllocationType::Linear> mDeps;
  VkRenderPassCreateInfo mCreateInfo;
  int32_t mDepthIdx;

  friend class VulkanRenderPass;
};

class VulkanRenderPass {
public:
  VulkanRenderPass();

  void init(
    const VulkanDevice &device,
    VulkanRenderPassConfig &config);

  void destroy(const VulkanDevice &device);

private:
  VkRenderPass mRenderPass;
  Array<VkClearValue, AllocationType::Freelist> mClearValues;
  Array<VkAttachmentDescription, AllocationType::Freelist> mAttachments;
  Array<AttachmentType, AllocationType::Freelist> mAttachmentTypes;
  Array<VkSubpassDescription, AllocationType::Freelist> mSubpasses;
  Array<VkAttachmentReference, AllocationType::Freelist> mRefs;

  friend class VulkanFramebufferConfig;
  friend class VulkanImgui;
  friend class VulkanCommandBuffer;
  friend class VulkanPipelineConfig;
};

}
