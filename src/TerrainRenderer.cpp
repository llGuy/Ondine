#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include "TerrainRenderer.hpp"

namespace Ondine::Graphics {

void TerrainRenderer::init(
  VulkanContext &graphicsContext,
  const GBuffer &gbuffer) {
  /* Create shader */
  VulkanPipelineConfig pipelineConfig(
    {gbuffer.renderPass(), 0},
    VulkanShader{graphicsContext.device(), "res/spv/Terrain.vert.spv"},
    VulkanShader{graphicsContext.device(), "res/spv/Terrain.frag.spv"});

  pipelineConfig.enableDepthTesting();
  pipelineConfig.configurePipelineLayout(
    sizeof(glm::mat4),
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

  pipelineConfig.configureVertexInput(2, 1);
  pipelineConfig.setBinding(
    0, sizeof(glm::vec3) * 2, VK_VERTEX_INPUT_RATE_VERTEX);
  pipelineConfig.setBindingAttribute(
    0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
  pipelineConfig.setBindingAttribute(
    1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3));

  pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  mPipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);

  mGPUVerticesAllocator.init(
    5000,
    (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer,
    graphicsContext);
}

void TerrainRenderer::render(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  Terrain &terrain,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline);
  commandBuffer.bindUniforms(
    camera.uniform(), planet.uniform(), clipping.uniform);

  for (int i = 0; i < terrain.mLoadedChunks.size; ++i) {
    const Chunk *chunk = terrain.mLoadedChunks[i];
    if (chunk->verticesMemory.size()) {
      commandBuffer.bindVertexBuffersArena(chunk->verticesMemory);
      glm::mat4 translate = glm::scale(glm::vec3(terrain.mTerrainScale)) *
        glm::translate(terrain.chunkCoordToWorld(chunk->chunkCoord));
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(chunk->vertexCount, 1, 0, 0);
    }
  }
}

void TerrainRenderer::sync(
  Terrain &terrain,
  const VulkanCommandBuffer &commandBuffer) {
  if (terrain.mUpdated) {
    terrain.mUpdated = false;
    terrain.generateVoxelNormals();

    uint32_t totalCount = 0;

    // Later change this to just the updated chunks
    for (int i = 0; i < terrain.mLoadedChunks.size; ++i) {
      Chunk *chunk = terrain.mLoadedChunks[i];
      uint32_t vertexCount = 0;
      ChunkVertex *vertices = terrain.createChunkVertices(*chunk, &vertexCount);

      chunk->vertexCount = vertexCount;

      totalCount += vertexCount;

      if (chunk->verticesMemory.size()) {
        // This chunk already has allocated memory
        mGPUVerticesAllocator.free(chunk->verticesMemory);
      }

      if (vertexCount) {
        auto slot = mGPUVerticesAllocator.allocate(
          sizeof(ChunkVertex) * vertexCount);

        slot.write(commandBuffer, vertices, sizeof(ChunkVertex) * vertexCount);

        chunk->verticesMemory = slot;
      }
      else {
        chunk->verticesMemory = {};
      }
    }

    LOG_INFOV("Total size: %d\n", totalCount * sizeof(ChunkVertex));
  }
}

}
