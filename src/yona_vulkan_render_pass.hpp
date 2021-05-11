#pragma once

#include "yona_utils.hpp"
#include "yona_buffer.hpp"
#include <vulkan/vulkan.h>

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
  ShaderRead
};

enum class AttachmentType {
  Color,
  Depth
};

/* This is always going to be a temporary object */
class VulkanRenderPassConfig {
public:
  VulkanRenderPassConfig() = default;

  void beginConfiguration(uint32_t attachmentCount, uint32_t subpassCount);

  void addAttachment(
    LoadAndStoreOp loadAndStore, LoadAndStoreOp stencilLoadAndStore,
    OutputUsage outputUsage, AttachmentType type, VkFormat format);

  void addSubpass(
    const Array<uint32_t, AllocationType::Linear> &colors,
    const Array<uint32_t, AllocationType::Linear> &inputs,
    bool hasDepth);

  void endConfiguration();

private:
  Array<VkAttachmentDescription, AllocationType::Linear> mAttachments;
  Array<AttachmentType, AllocationType::Linear> mAttachmentTypes;
  int32_t mDepthIdx;
  Array<VkSubpassDescription, AllocationType::Linear> mSubpasses;
  Array<VkAttachmentReference, AllocationType::Linear> mRefs;
  Array<VkSubpassDependency, AllocationType::Linear> mDeps;
  VkRenderPassCreateInfo mCreateInfo;

  friend class VulkanRenderPass;
};

class VulkanRenderPass {
public:
  VulkanRenderPass() = default;

  void init(const VulkanRenderPassConfig &config);

private:
  VkRenderPass mRenderPass;
};

}
