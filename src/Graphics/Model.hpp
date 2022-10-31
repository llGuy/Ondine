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

class Geometry {
public:
  struct Vertices {
    const glm::vec3 *vertices;
    size_t count;
  };

  virtual Vertices getVertices() const = 0;

  struct Indices {
    const uint32_t *indices;
    size_t count;
  };

  virtual Indices getIndices() const = 0;
};

class ModelConfig : public Geometry {
public:
  ModelConfig() = default;
  ModelConfig(uint32_t vertexCount);

  void pushAttribute(const Attribute &attribute, const Buffer &data);
  void configureIndices(
    uint32_t indexCount, VkIndexType type, const Buffer &data);

  void configureVertexInput(VulkanPipelineConfig &config);

  Vertices getVertices() const override;
  Indices getIndices() const override;

protected:
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
  friend class AnimationManager;
  friend class std::hash<ModelConfig>;
};

class Model {
public:
  void init(const ModelConfig &config, VulkanContext &context);

  void bindVertexBuffers(const VulkanCommandBuffer &commandBuffer) const;
  void bindIndexBuffer(const VulkanCommandBuffer &commandBuffer) const;
  void submitForRenderIndexed(
    const VulkanCommandBuffer &commandBuffer,
    uint32_t instanceCount = 1) const;

  void submitForRender(
    const VulkanCommandBuffer &commandBuffer,
    uint32_t instanceCount = 1) const;

private:
  static constexpr uint32_t MAX_VERTEX_BUFFER_COUNT = 10;

  uint32_t mVertexBufferCount;
  VulkanBuffer mVertexBuffers[MAX_VERTEX_BUFFER_COUNT];
  VkBuffer mVertexBuffersRaw[MAX_VERTEX_BUFFER_COUNT];

  VulkanBuffer mIndexBuffer;
  VkIndexType mIndexType;
  uint32_t mIndexCount;

  uint32_t mVertexCount;
};

}

namespace std {

template <>
struct hash<Ondine::Graphics::ModelConfig> {
  std::size_t operator()(const Ondine::Graphics::ModelConfig &cfg) const {
    const uint64_t PRIME_NUMBER = 7873;

    uint64_t finalHash = 0xC0FFEEBEEF;
    finalHash = finalHash * PRIME_NUMBER + cfg.mAttributeCount;

    for (int i = 0; i < cfg.mAttributeCount; i++) {
      finalHash = finalHash * PRIME_NUMBER + cfg.mAttributes[i].attribSize;
      finalHash = finalHash * PRIME_NUMBER + (uint64_t)cfg.mAttributes[i].format;
    }

    return finalHash;
  }
};

}
