#include "Vulkan.hpp"
#include "VulkanDevice.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanDescriptor.hpp"
#include "VulkanRenderPass.hpp"

namespace Yona {

VulkanShader::VulkanShader(
  const VulkanDevice &device,
  const Buffer &source, VulkanShaderType type)
  : mType(type) {
  VkShaderModuleCreateInfo shaderInfo = {};
  shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderInfo.codeSize = source.size;
  shaderInfo.pCode = (uint32_t *)source.data;

  VK_CHECK(
    vkCreateShaderModule(
      device.mLogicalDevice,
      &shaderInfo,
      NULL,
      &mModule));
}

void VulkanPipelineConfig::enableBlendingSame(
  uint32_t attachmentIndex,
  VkBlendOp op, VkBlendFactor src, VkBlendFactor dst) {
  auto &blendState = mBlendStates[attachmentIndex];

  blendState.blendEnable = VK_TRUE;
  blendState.colorBlendOp = op;
  blendState.alphaBlendOp = op;

  blendState.srcColorBlendFactor = src;
  blendState.srcAlphaBlendFactor = src;

  blendState.dstColorBlendFactor = dst;
  blendState.dstAlphaBlendFactor = dst;

  // Color write mask is already set
}

void VulkanPipelineConfig::setDefaultValues() {
  /* Blend states */
  const auto &renderPass = mTarget.renderPass;
  const auto &subpass = renderPass.mSubpasses[mTarget.subpassIndex];

  uint32_t colorAttachmentCount = subpass.colorAttachmentCount;
  mBlendStates.init(colorAttachmentCount);
  mBlendStates.zero();

  for (int i = 0; i < colorAttachmentCount; ++i) {
    const auto &colorRef = subpass.pColorAttachments[i];
    const auto &attachmentDesc = renderPass.mAttachments[colorRef.attachment];
    auto &blendState = mBlendStates[mBlendStates.size++];

    switch (attachmentDesc.format) {
      // TODO: Add other formats as they come up
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT: {
      blendState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    } break;

    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT: {
      blendState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;
    } break;

    case VK_FORMAT_R16_SFLOAT: {
      blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    } break;

    default: {
      LOG_ERROR("Handling unsupported format for color blending!\n");
      PANIC_AND_EXIT();
    } break;
    }
  }

  mBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  mBlending.logicOpEnable = VK_FALSE;
  mBlending.attachmentCount = colorAttachmentCount;
  mBlending.pAttachments = mBlendStates.data;
  mBlending.logicOp = VK_LOGIC_OP_COPY;

  /* Vertex input (empty by default) */
  mVertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  /* Input assembly (triangle strip by default) */
  mInputAssembly.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  mInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

  /* Viewport */
  mViewport.width = 1;
  mViewport.height = 1;
  mViewport.maxDepth = 1.0f;

  mRect.extent = {1, 1};
    
  mViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  mViewportInfo.viewportCount = 1;
  mViewportInfo.pViewports = &mViewport;
  mViewportInfo.scissorCount = 1;
  mViewportInfo.pScissors = &mRect;

  /* Rasterization */
  mRasterization.sType =
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  mRasterization.polygonMode = VK_POLYGON_MODE_FILL;
  mRasterization.cullMode = VK_CULL_MODE_NONE;
  mRasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
  mRasterization.lineWidth = 1.0f;

  /* Multisampling */
  mMultisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  mMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  mMultisample.minSampleShading = 1.0f;

  /* Dynamic states */
  mDynamicStates = makeArray<VkDynamicState, AllocationType::Linear>(
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR);

  mDynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  mDynamicState.dynamicStateCount = mDynamicStates.size;
  mDynamicState.pDynamicStates = mDynamicStates.data;

  /* Depth stencil - off by default */
  mDepthStencil.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  mDepthStencil.depthTestEnable = VK_FALSE;
  mDepthStencil.depthWriteEnable = VK_FALSE;
}

void VulkanPipelineConfig::setShaderStages(
  const Array<VulkanShader, AllocationType::Linear> &shaders) {
  for (int i = 0; i < shaders.size; ++i) {
    VkPipelineShaderStageCreateInfo *info = &mShaderStages[i];
    info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info->pName = "main";
    info->stage = (VkShaderStageFlagBits)shaders[i].mType;
    info->module = shaders[i].mModule;
  }

  mShaderStages.size = shaders.size;
}

void VulkanPipelineConfig::finishConfiguration(
  const VulkanDevice &device,
  VulkanDescriptorSetLayoutMaker &layout) {
  /* Create pipeline layout */
  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL;
  pushConstantRange.offset = 0;
  pushConstantRange.size = mPushConstantSize;

  VkDescriptorSetLayout *layouts = STACK_ALLOC(
    VkDescriptorSetLayout, mLayouts.size);

  for (int i = 0; i < mLayouts.size; ++i) {
    layouts[i] = layout.getDescriptorSetLayout(
      device, mLayouts[i].type, mLayouts[i].count);
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = mLayouts.size;
  pipelineLayoutInfo.pSetLayouts = layouts;

  if (mPushConstantSize) {
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  }

  VK_CHECK(
    vkCreatePipelineLayout(
      device.mLogicalDevice,
      &pipelineLayoutInfo,
      NULL,
      &mPipelineLayout));

  mCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  mCreateInfo.stageCount = mShaderStages.size;
  mCreateInfo.pStages = mShaderStages.data;
  mCreateInfo.pVertexInputState = &mVertexInput;
  mCreateInfo.pInputAssemblyState = &mInputAssembly;
  mCreateInfo.pViewportState = &mViewportInfo;
  mCreateInfo.pRasterizationState = &mRasterization;
  mCreateInfo.pMultisampleState = &mMultisample;
  mCreateInfo.pDepthStencilState = &mDepthStencil;
  mCreateInfo.pColorBlendState = &mBlending;
  mCreateInfo.pDynamicState = &mDynamicState;
  mCreateInfo.layout = mPipelineLayout;
  mCreateInfo.renderPass = mTarget.renderPass.mRenderPass;
  mCreateInfo.subpass = mTarget.subpassIndex;
  mCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  mCreateInfo.basePipelineIndex = -1;
}

void VulkanPipeline::init(
  const VulkanDevice &device,
  VulkanDescriptorSetLayoutMaker &layouts,
  VulkanPipelineConfig &config) {
  config.finishConfiguration(device, layouts);

  VK_CHECK(
    vkCreateGraphicsPipelines(
      device.mLogicalDevice,
      VK_NULL_HANDLE,
      1,
      &config.mCreateInfo,
      NULL,
      &mPipeline));

  mPipelineLayout = config.mPipelineLayout;
}

void VulkanPipeline::destroy(const VulkanDevice &device) {
  vkDestroyPipeline(device.mLogicalDevice, mPipeline, nullptr);
  vkDestroyPipelineLayout(device.mLogicalDevice, mPipelineLayout, nullptr);
}

}
