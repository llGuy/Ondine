#pragma once

#include <stdint.h>
#include "VulkanBuffer.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Graphics {

struct Attribute {
  uint32_t size;
  VkFormat format;
  VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
};

class ModelConfig {
public:
  ModelConfig() = default;
  ModelConfig(uint32_t vertexCount);

  void pushAttribute(const Attribute &attribute, const Buffer &data);
  void configureIndices(
    uint32_t indexCount, VkIndexType type, const Buffer &data);

  void configureVertexInput(VulkanPipelineConfig &config);

private:
  static constexpr uint32_t MAX_ATTRIBUTE_COUNT = 10;

  struct ModelAttribute {
    uint32_t attribSize;
    VkFormat format;

    Buffer data;
  };

  uint32_t mVertexCount;
  uint32_t mAttributeCount;
  ModelAttribute mAttributes[MAX_ATTRIBUTE_COUNT];

  uint32_t mIndexCount;
  VkIndexType mIndexType;
  Buffer mIndices;

  friend class Model;
};

class Model {
public:
  void init(const ModelConfig &config, VulkanContext &context);

  void bindVertexBuffers(const VulkanCommandBuffer &commandBuffer);
  void bindIndexBuffer(const VulkanCommandBuffer &commandBuffer);
  void submitForRender(const VulkanCommandBuffer &commandBuffer);

private:
  static constexpr uint32_t MAX_VERTEX_BUFFER_COUNT = 10;

  uint32_t mVertexBufferCount;
  VulkanBuffer mVertexBuffers[MAX_VERTEX_BUFFER_COUNT];
  VkBuffer mVertexBuffersRaw[MAX_VERTEX_BUFFER_COUNT];

  VulkanBuffer mIndexBuffer;
  VkIndexType mIndexType;
  uint32_t mIndexCount;
};

}
