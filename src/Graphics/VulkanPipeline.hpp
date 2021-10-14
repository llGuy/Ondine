#pragma once

#include <vulkan/vulkan.h>
#include "Buffer.hpp"

namespace Ondine::Graphics {

class VulkanDevice;
class VulkanRenderPass;
class VulkanDescriptorSetLayoutMaker;

enum VulkanShaderType {
  Vertex = VK_SHADER_STAGE_VERTEX_BIT,
  Geometry = VK_SHADER_STAGE_GEOMETRY_BIT,
  Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
  Compute = VK_SHADER_STAGE_COMPUTE_BIT
};

class VulkanShader {
public:
  // To uncomment when I feel like replacing calls to the shader constructor */
  /* [[deprecated]] */ VulkanShader(
    const VulkanDevice &device,
    const Buffer &source, VulkanShaderType type);

  VulkanShader(const VulkanDevice &device, const char *path);

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
      mCreateInfoGraphics{} {
    mShaderStages.zero();
    setShaderStages(
      makeArray<VulkanShader, AllocationType::Linear>(shaders...));

    setDefaultValues();
  }

  void enableBlendingSame(
    uint32_t attachmentIndex,
    VkBlendOp op, VkBlendFactor src, VkBlendFactor dst);

  void enableDepthTesting();

  template <typename ...T>
  void configurePipelineLayout(
    size_t pushConstantSize,
    T ...layouts) {
    mPushConstantSize = pushConstantSize;
    mLayouts =
      makeArray<VulkanPipelineDescriptorLayout, AllocationType::Linear>(
        layouts...);
  }

  void configureVertexInput(uint32_t attribCount, uint32_t bindingCount);

  void setBinding(
    uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);

  void setBindingAttribute(
    uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

  void setTopology(VkPrimitiveTopology topology);
  void setToWireframe();

private:
  void setDefaultValues();

  void setShaderStages(
    const Array<VulkanShader, AllocationType::Linear> &sources);

  void finishConfiguration(
    const VulkanDevice &device,
    VulkanDescriptorSetLayoutMaker &layout);

  bool isCompute();

private:
  VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
  VkPipelineVertexInputStateCreateInfo mVertexInput;
  Array<VkVertexInputAttributeDescription, AllocationType::Linear> mAttributes;
  Array<VkVertexInputBindingDescription, AllocationType::Linear> mBindings;
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
  Array<VulkanPipelineDescriptorLayout, AllocationType::Linear> mLayouts;
  Array<VkPipelineShaderStageCreateInfo, AllocationType::Linear> mShaderStages;
  size_t mPushConstantSize;
  VulkanShaderTarget mTarget;

  union {
    VkGraphicsPipelineCreateInfo mCreateInfoGraphics;
    VkComputePipelineCreateInfo mCreateInfoCompute;
  };

  VkPipelineLayout mPipelineLayout;
  bool isComputePipeline;

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
