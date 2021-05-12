#pragma once

#include "yona_utils.hpp"
#include "yona_buffer.hpp"
#include <vulkan/vulkan.h>
#include "yona_vulkan_device.hpp"

namespace Yona {

enum class LoadAndStoreOp {
  ClearThenStore = BIT(0) | BIT(2),
  ClearThenDontCare = BIT(0) | BIT(3),

  DontCareThenStore = BIT(1) | BIT(2),
  DontCareThenDontCare = BIT(1) | BIT(3)
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

enum class AttachmentType {
  Color,
  Depth
};

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
  Array<VkAttachmentDescription, AllocationType::Linear> mAttachments;
  Array<AttachmentType, AllocationType::Linear> mAttachmentTypes;
  Array<OutputUsage, AllocationType::Linear> mOutputUsages;
  Array<VkSubpassDescription, AllocationType::Linear> mSubpasses;
  Array<VkAttachmentReference, AllocationType::Linear> mRefs;
  Array<VkSubpassDependency, AllocationType::Linear> mDeps;
  VkRenderPassCreateInfo mCreateInfo;
  int32_t mDepthIdx;

  friend class VulkanRenderPass;
};

class VulkanRenderPass {
public:
  VulkanRenderPass() = default;

  void init(
    const VulkanDevice &device,
    VulkanRenderPassConfig &config);

private:
  VkRenderPass mRenderPass;

  friend class VulkanFramebufferConfig;
};

}
