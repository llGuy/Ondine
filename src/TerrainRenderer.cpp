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
    10000,
    (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer,
    graphicsContext);

  VulkanPipelineConfig lineConfig(
    {gbuffer.renderPass(), 0},
    VulkanShader{graphicsContext.device(), "res/spv/Line.vert.spv"},
    VulkanShader{graphicsContext.device(), "res/spv/Line.frag.spv"});

  lineConfig.enableDepthTesting();
  lineConfig.configurePipelineLayout(
    sizeof(glm::vec4) + sizeof(glm::vec4) * 2 + sizeof(glm::vec4),
    VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
  lineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

  mRenderLine.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    lineConfig);
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

void TerrainRenderer::renderChunkOutlines(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  Terrain &terrain,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mRenderLine);
  commandBuffer.bindUniforms(camera.uniform());

  static const glm::vec4 CUBE_POSITIONS[8] = {
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
    glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
    glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),

    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
  };

  struct {
    glm::vec4 transform;
    glm::vec4 positions[2];
    glm::vec4 color;
  } pushConstant;

  auto renderLine = [&commandBuffer, &pushConstant]() {
    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);
    commandBuffer.draw(2, 1, 0, 0);
  };

  pushConstant.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

  for (int i = 0; i < terrain.mLoadedChunks.size; ++i) {
    const Chunk *chunk = terrain.mLoadedChunks[i];
    if (chunk->verticesMemory.size()) {
      float scale = (float)terrain.mTerrainScale * (float)CHUNK_DIM;
        
      pushConstant.transform = glm::vec4(
        (float)chunk->chunkCoord.x,
        (float)chunk->chunkCoord.y,
        (float)chunk->chunkCoord.z,
        scale);

      pushConstant.positions[0] = CUBE_POSITIONS[0];
      pushConstant.positions[1] = CUBE_POSITIONS[4];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[1];
      pushConstant.positions[1] = CUBE_POSITIONS[5];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[2];
      pushConstant.positions[1] = CUBE_POSITIONS[6];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[3];
      pushConstant.positions[1] = CUBE_POSITIONS[7];
      renderLine();

      pushConstant.positions[0] = CUBE_POSITIONS[0];
      pushConstant.positions[1] = CUBE_POSITIONS[1];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[0];
      pushConstant.positions[1] = CUBE_POSITIONS[2];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[1];
      pushConstant.positions[1] = CUBE_POSITIONS[3];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[2];
      pushConstant.positions[1] = CUBE_POSITIONS[3];
      renderLine();

      pushConstant.positions[0] = CUBE_POSITIONS[4];
      pushConstant.positions[1] = CUBE_POSITIONS[5];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[4];
      pushConstant.positions[1] = CUBE_POSITIONS[6];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[5];
      pushConstant.positions[1] = CUBE_POSITIONS[7];
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[6];
      pushConstant.positions[1] = CUBE_POSITIONS[7];
      renderLine();
    }
  }
}

void TerrainRenderer::renderQuadTree(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  Terrain &terrain,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mRenderLine);
  commandBuffer.bindUniforms(camera.uniform());

  renderQuadTreeNode(
    terrain.mQuadTree.mRoot,
    glm::ivec2(0),
    camera,
    planet,
    clipping,
    terrain,
    frame);
}

void TerrainRenderer::renderQuadTreeNode(
  QuadTree::Node *node,
  const glm::ivec2 &offset,
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  Terrain &terrain,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  static const glm::vec4 POSITIONS[4] = {
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(1.0f, 0.0f, 1.0f, 1.0f),
  };

  struct {
    glm::vec4 transform;
    glm::vec4 positions[2];
    glm::vec4 color;
  } pushConstant;

  auto renderLine = [&commandBuffer, &pushConstant]() {
    commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);
    commandBuffer.draw(2, 1, 0, 0);
  };

  pushConstant.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
  pushConstant.transform = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

  glm::ivec2 wOffset = terrain.quadTreeCoordsToWorld(offset);
  float width = (float)(pow(2, terrain.mQuadTree.mMaxLOD - node->level) *
    CHUNK_DIM * terrain.mTerrainScale);

  glm::vec4 realOffset = glm::vec4(wOffset.x, 100.0f, wOffset.y, 0.0f);

  pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
  pushConstant.positions[1] = POSITIONS[1] * width + realOffset;
  renderLine();
  pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
  pushConstant.positions[1] = POSITIONS[2] * width + realOffset;
  renderLine();
  pushConstant.positions[0] = POSITIONS[1] * width + realOffset;
  pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
  renderLine();
  pushConstant.positions[0] = POSITIONS[2] * width + realOffset;
  pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
  renderLine();

  if (node->level < terrain.mQuadTree.mMaxLOD) {
    int widthUnder = pow(2, terrain.mQuadTree.mMaxLOD - node->level - 1);
    glm::ivec2 offsets[4] = {
      offset,
      offset + glm::ivec2(widthUnder, 0),
      offset + glm::ivec2(0, widthUnder),
      offset + glm::ivec2(widthUnder),
    };

    for (int i = 0; i < 4; ++i) {
      if (node->children[i]) {
        renderQuadTreeNode(
          node->children[i],
          offsets[i],
          camera,
          planet,
          clipping,
          terrain,
          frame);
      }
    }
  }
}

void TerrainRenderer::sync(
  Terrain &terrain,
  const VulkanCommandBuffer &commandBuffer) {
  if (terrain.mUpdatedChunks.size) {
    terrain.generateVoxelNormals();

    uint32_t totalCount = 0;

    // Later change this to just the updated chunks
    for (int i = 0; i < terrain.mUpdatedChunks.size; ++i) {
      uint32_t chunkIndex = terrain.mUpdatedChunks[i];
      Chunk *chunk = terrain.mLoadedChunks[chunkIndex];
      chunk->needsUpdating = false;
      
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

    // LOG_INFOV("Total size: %u\n", (uint32_t)(totalCount * sizeof(ChunkVertex)));

    terrain.mUpdatedChunks.size = 0;

    mGPUVerticesAllocator.debugLogState();
  }
}

}
