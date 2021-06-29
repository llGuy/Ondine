#include <assert.h>
#include "Model.hpp"

namespace Ondine::Graphics {

ModelConfig::ModelConfig(uint32_t vertexCount)
  : mVertexCount(vertexCount),
    mAttributeCount(0),
    mAttributes{},
    mIndexCount(0),
    mIndexType(VkIndexType(0)),
    mIndices{} {
  
}

void ModelConfig::pushAttribute(
  const Attribute &attribute, const Buffer &data) {
  assert(mAttributeCount < MAX_ATTRIBUTE_COUNT);
  mAttributes[mAttributeCount++] = {
    attribute.size,
    attribute.format,
    data
  };
}

void ModelConfig::configureIndices(
  uint32_t indexCount, VkIndexType type, const Buffer &data) {
  mIndexCount = indexCount;
  mIndexType = type;
  mIndices = data;
}

void ModelConfig::configureVertexInput(VulkanPipelineConfig &config) {
  config.configureVertexInput(mAttributeCount, mAttributeCount);
  for (int i = 0; i < mAttributeCount; ++i) {
    config.setBinding(i, mAttributes[i].attribSize, VK_VERTEX_INPUT_RATE_VERTEX);
    config.setBindingAttribute(i, i, mAttributes[i].format, 0);
  }
}

void Model::init(const ModelConfig &def, VulkanContext &context) {
  mVertexCount = def.mVertexCount;

  if (def.mIndexCount > 0) {
    mIndexBuffer.init(
      context.device(), def.mIndices.size,
      (VulkanBufferFlagBits)VulkanBufferFlag::IndexBuffer);

    mIndexBuffer.fillWithStaging(
      context.device(), context.commandPool(),
      def.mIndices);
  }

  // For now, store each attribute in a separate vertex buffer
  for (int i = 0; i < def.mAttributeCount; ++i) {
    auto &attribute = def.mAttributes[i];
    auto &buf = mVertexBuffers[mVertexBufferCount++];

    buf.init(
      context.device(), attribute.data.size,
      (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer);

    buf.fillWithStaging(
      context.device(), context.commandPool(),
      attribute.data);

    mVertexBuffersRaw[i] = buf.mBuffer;
  }

  mVertexBufferCount = def.mAttributeCount;
  mIndexType = def.mIndexType;
  mIndexCount = def.mIndexCount;
}

void Model::bindVertexBuffers(const VulkanCommandBuffer &commandBuffer) const {
  VkDeviceSize *offsets = STACK_ALLOC(VkDeviceSize, mVertexBufferCount);
  memset(offsets, 0, sizeof(VkDeviceSize) * mVertexBufferCount);

  commandBuffer.bindVertexBuffers(
    0, mVertexBufferCount, mVertexBuffersRaw, offsets);
}

void Model::bindIndexBuffer(const VulkanCommandBuffer &commandBuffer) const {
  commandBuffer.bindIndexBuffer(0, mIndexType, mIndexBuffer);
}

void Model::submitForRenderIndexed(
  const VulkanCommandBuffer &commandBuffer,
  uint32_t instanceCount) {
  commandBuffer.drawIndexed(mIndexCount, instanceCount, 0, 0, 0);
}

void Model::submitForRender(
  const VulkanCommandBuffer &commandBuffer,
  uint32_t instanceCount) const {
  commandBuffer.draw(mVertexCount, instanceCount, 0, 0);
}

}
