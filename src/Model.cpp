#include <assert.h>
#include "Model.hpp"

namespace Ondine::Graphics {

ModelConfig::ModelConfig(uint32_t vertexCount)
  : mVertexCount(vertexCount) {
  
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

void Model::init(const ModelConfig &def, VulkanContext &context) {
  if (def.mIndices.size) {
    mIndexBuffer.init(
      context.device(), def.mIndices.size,
      (VulkanBufferFlagBits)VulkanBufferFlag::IndexBuffer);

    mIsUsingIndexBuffer = true;
  }
  else {
    mIsUsingIndexBuffer = false;
  }

  // For now, store each attribute in a separate vertex buffer
  for (int i = 0; i < def.mAttributeCount; ++i) {
    VulkanBuffer &buf = mVertexBuffers[mVertexBufferCount++];

  }
}

}
