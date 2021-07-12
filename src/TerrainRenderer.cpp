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

  pipelineConfig.configureVertexInput(1, 1);
  pipelineConfig.setBinding(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX);
  pipelineConfig.setBindingAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

  pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  mPipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);
}

void TerrainRenderer::render(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  const Terrain &terrain,
  VulkanFrame &frame) const {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipeline);
  commandBuffer.bindUniforms(
    camera.uniform(), planet.uniform(), clipping.uniform);

  for (int i = 0; i < terrain.mLoadedChunks.size; ++i) {
    const Chunk *chunk = terrain.mLoadedChunks[i];
    if (chunk->vertices.vertexCount) {
      commandBuffer.bindVertexBuffers(0, 1, &chunk->vertices.vbo);
      glm::mat4 translate = glm::translate(
        terrain.chunkCoordToWorld(chunk->chunkCoord));
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(chunk->vertices.vertexCount, 1, 0, 0);
    }
  }
}

}
