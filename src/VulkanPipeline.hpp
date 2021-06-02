#pragma once

#include <vulkan/vulkan.h>
#include "Buffer.hpp"

namespace Ondine {

class VulkanDevice;
class VulkanRenderPass;
class VulkanDescriptorSetLayoutMaker;

enum VulkanShaderType {
  Vertex = VK_SHADER_STAGE_VERTEX_BIT,
  Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
  Fragment = VK_SHADER_STAGE_FRAGMENT_BIT
};

class VulkanShader {
public:
  VulkanShader(
    const VulkanDevice &device,
    const Buffer &source, VulkanShaderType type);

private:
  VkShaderModule mModule;
  VulkanShaderType mType;

  friend class VulkanPipelineConfig;
};

struct VulkanShaderTarget {
  const VulkanRenderPass &renderPass;
  uint32_t subpassIndex;
};

struct VulkanPipelineDescriptorLayout {
  VkDescriptorType type;
  size_t count;
};

// There will of course be default values so that not everything needs to be set
class VulkanPipelineConfig {
public:
  // With T = [VulkanShader, VulkanShader, ...]
  template <typename ...T>
  VulkanPipelineConfig(const VulkanShaderTarget &target, T ...shaders)
    : mInputAssembly{},
      mVertexInput{},
      mViewport{},
      mRasterization{},
      mMultisample{},
      mBlending{},
      mDynamicState{},
      mDepthStencil{},
      mLayouts{},
      mShaderStages(sizeof...(shaders)),
      mPushConstantSize(0),
      mViewportInfo{},
      mTarget(target),
      mCreateInfo{} {
    mShaderStages.zero();

    setDefaultValues();

    setShaderStages(
      makeArray<VulkanShader, AllocationType::Linear>(shaders...));
  }

  void enableBlendingSame(
    uint32_t attachmentIndex,
    VkBlendOp op, VkBlendFactor src, VkBlendFactor dst);

  // void enableBlendingSeparate();

  template <typename ...T>
  void configurePipelineLayout(
    size_t pushConstantSize,
    T ...layouts) {
    mPushConstantSize = pushConstantSize;
    mLayouts =
      makeArray<VulkanPipelineDescriptorLayout, AllocationType::Linear>(
        layouts...);
  }

private:
  void setDefaultValues();

  void setShaderStages(
    const Array<VulkanShader, AllocationType::Linear> &sources);

  void finishConfiguration(
    const VulkanDevice &device,
    VulkanDescriptorSetLayoutMaker &layout);

private:
  VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
  VkPipelineVertexInputStateCreateInfo mVertexInput;
  VkViewport mViewport;
  VkRect2D mRect;
  VkPipelineViewportStateCreateInfo mViewportInfo;
  VkPipelineRasterizationStateCreateInfo mRasterization;
  VkPipelineMultisampleStateCreateInfo mMultisample;
  VkPipelineColorBlendStateCreateInfo mBlending;
  Array<
    VkPipelineColorBlendAttachmentState,
    AllocationType::Linear> mBlendStates;
  VkPipelineDynamicStateCreateInfo mDynamicState;
  Array<VkDynamicState, AllocationType::Linear> mDynamicStates;
  VkPipelineDepthStencilStateCreateInfo mDepthStencil;
  Array<VkPipelineShaderStageCreateInfo, AllocationType::Linear> mShaderStages;
  size_t mPushConstantSize;
  Array<VulkanPipelineDescriptorLayout, AllocationType::Linear> mLayouts;
  VulkanShaderTarget mTarget;
  VkGraphicsPipelineCreateInfo mCreateInfo;
  VkPipelineLayout mPipelineLayout;

  friend class VulkanPipeline;
};

/* 
   Will need to set the viewport in the command buffer as the default
   values will be 1 so that no viewport needs to be specified when creating
   the pipeline.
*/
class VulkanPipeline {
public:
  void init(
    const VulkanDevice &device,
    VulkanDescriptorSetLayoutMaker &layouts,
    VulkanPipelineConfig &config);

  void destroy(const VulkanDevice &device);

private:
  VkPipeline mPipeline;
  VkPipelineLayout mPipelineLayout;

  friend class VulkanCommandBuffer;
};

}
