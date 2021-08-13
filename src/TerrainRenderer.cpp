#include "Math.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include "TerrainRenderer.hpp"

namespace Ondine::Graphics {

const glm::vec3 TerrainRenderer::NORMALIZED_CUBE_VERTICES[8] = {
  glm::vec3(-0.5f, -0.5f, -0.5f),
  glm::vec3(+0.5f, -0.5f, -0.5f),
  glm::vec3(-0.5f, +0.5f, -0.5f),
  glm::vec3(+0.5f, +0.5f, -0.5f),
  glm::vec3(-0.5f, -0.5f, +0.5f),
  glm::vec3(+0.5f, -0.5f, +0.5f),
  glm::vec3(-0.5f, +0.5f, +0.5f),
  glm::vec3(+0.5f, +0.5f, +0.5f),
};

const glm::ivec3 TerrainRenderer::NORMALIZED_CUBE_VERTEX_INDICES[8] = {
  glm::ivec3(0, 0, 0),
  glm::ivec3(1, 0, 0),
  glm::ivec3(0, 1, 0),
  glm::ivec3(1, 1, 0),
  glm::ivec3(0, 0, 1),
  glm::ivec3(1, 0, 1),
  glm::ivec3(0, 1, 1),
  glm::ivec3(1, 1, 1),
};

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

  pipelineConfig.setToWireframe();

  mPipelineWireframe.init(
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

  mChunkGroups.init(1000);
  mChunkGroupIndices.init();
  mTemporaryVertices = flAllocv<ChunkVertex>(
    10 * CHUNK_DIM * CHUNK_DIM * CHUNK_DIM);
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

  for (auto group : mChunkGroups) {
    if (group->verticesMemory.size()) {
      commandBuffer.bindVertexBuffersArena(group->verticesMemory);

      float lodScale = glm::pow(2.0f, terrain.mQuadTree.mMaxLOD - group->level);
      glm::vec3 scale = glm::vec3(terrain.mTerrainScale) * lodScale;
      glm::vec3 tran = (glm::vec3)terrain.chunkCoordToWorld(group->coord) *
        (float)terrain.mTerrainScale;

      glm::mat4 translate = glm::translate(tran) * glm::scale(scale);

      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group->vertexCount, 1, 0, 0);
    }
  }
}

void TerrainRenderer::renderWireframe(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  Terrain &terrain,
  VulkanFrame &frame) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.bindPipeline(mPipelineWireframe);
  commandBuffer.bindUniforms(
    camera.uniform(), planet.uniform(), clipping.uniform);

  for (auto group : mChunkGroups) {
    if (group->verticesMemory.size()) {
      commandBuffer.bindVertexBuffersArena(group->verticesMemory);

      float lodScale = glm::pow(2.0f, terrain.mQuadTree.mMaxLOD - group->level);
      glm::vec3 scale = glm::vec3(terrain.mTerrainScale) * lodScale;
      glm::vec3 tran = (glm::vec3)terrain.chunkCoordToWorld(group->coord) *
        (float)terrain.mTerrainScale;

      glm::mat4 translate = glm::translate(tran) * glm::scale(scale);

      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group->vertexCount, 1, 0, 0);
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

  for (auto group : mChunkGroups) {
    if (group->verticesMemory.size()) {
      float scale = (float)terrain.mTerrainScale * (float)CHUNK_DIM;
        
      pushConstant.transform = glm::vec4(
        (float)group->coord.x,
        (float)group->coord.y,
        (float)group->coord.z,
        scale);

      float stretch = glm::pow(2.0f, terrain.mQuadTree.mMaxLOD - group->level);

      pushConstant.positions[0] = CUBE_POSITIONS[0] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[4] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[1] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[5] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[2] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[6] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[3] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[7] * stretch;
      renderLine();

      pushConstant.positions[0] = CUBE_POSITIONS[0] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[1] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[0] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[2] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[1] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[3] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[2] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[3] * stretch;
      renderLine();

      pushConstant.positions[0] = CUBE_POSITIONS[4] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[5] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[4] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[6] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[5] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[7] * stretch;
      renderLine();
      pushConstant.positions[0] = CUBE_POSITIONS[6] * stretch;
      pushConstant.positions[1] = CUBE_POSITIONS[7] * stretch;
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

  for (int i = 0; i < terrain.mQuadTree.mDeepestNodes.size; ++i) {
    QuadTree::Node *node = terrain.mQuadTree.mDeepestNodes[i];

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

    // glm::ivec2 wOffset = terrain.quadTreeCoordsToWorld(offset);
    glm::ivec2 wOffset = terrain.quadTreeCoordsToWorld(
      glm::ivec2(node->offsetx, node->offsety));
 
    float width = (float)(pow(2, terrain.mQuadTree.mMaxLOD - node->level) *
                          CHUNK_DIM * terrain.mTerrainScale);

    glm::vec4 realOffset = glm::vec4(wOffset.x, 100.0f, wOffset.y, 0.0f);

    pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
    pushConstant.positions[0].w = 1.0f;
    pushConstant.positions[1] = POSITIONS[1] * width + realOffset;
    pushConstant.positions[1].w = 1.0f;
    renderLine();
    pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
    pushConstant.positions[0].w = 1.0f;
    pushConstant.positions[1] = POSITIONS[2] * width + realOffset;
    pushConstant.positions[1].w = 1.0f;
    renderLine();
    pushConstant.positions[0] = POSITIONS[1] * width + realOffset;
    pushConstant.positions[0].w = 1.0f;
    pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
    pushConstant.positions[1].w = 1.0f;
    renderLine();
    pushConstant.positions[0] = POSITIONS[2] * width + realOffset;
    pushConstant.positions[0].w = 1.0f;
    pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
    pushConstant.positions[1].w = 1.0f;
    renderLine();
  }
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

  // glm::ivec2 wOffset = terrain.quadTreeCoordsToWorld(offset);
  glm::ivec2 wOffset = terrain.quadTreeCoordsToWorld(
    glm::ivec2(node->offsetx, node->offsety));
 
  float width = (float)(pow(2, terrain.mQuadTree.mMaxLOD - node->level) *
    CHUNK_DIM * terrain.mTerrainScale);

  glm::vec4 realOffset = glm::vec4(wOffset.x, 100.0f, wOffset.y, 0.0f);

  pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
  pushConstant.positions[0].w = 1.0f;
  pushConstant.positions[1] = POSITIONS[1] * width + realOffset;
  pushConstant.positions[1].w = 1.0f;
  renderLine();
  pushConstant.positions[0] = POSITIONS[0] * width + realOffset;
  pushConstant.positions[0].w = 1.0f;
  pushConstant.positions[1] = POSITIONS[2] * width + realOffset;
  pushConstant.positions[1].w = 1.0f;
  renderLine();
  pushConstant.positions[0] = POSITIONS[1] * width + realOffset;
  pushConstant.positions[0].w = 1.0f;
  pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
  pushConstant.positions[1].w = 1.0f;
  renderLine();
  pushConstant.positions[0] = POSITIONS[2] * width + realOffset;
  pushConstant.positions[0].w = 1.0f;
  pushConstant.positions[1] = POSITIONS[3] * width + realOffset;
  pushConstant.positions[1].w = 1.0f;
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
    terrain.mUpdatedChunks.size = 0;

    /* 
       Trying the naive way first
       Goal is to generate the voxel data for each ChunkGroup
       After the voxel information for each ChunkGroup is created, we
       can then proceed to generating the mesh for each ChunkGroup
    */
    for (int i = 0; i < terrain.mQuadTree.nodeCount(); ++i) {
      QuadTree::Node *node = terrain.mQuadTree.mDeepestNodes[i];
      glm::ivec2 offset = terrain.quadTreeCoordsToChunk(
        glm::ivec2(node->offsetx, node->offsety));
      int width = pow(2, terrain.mQuadTree.mMaxLOD - node->level);

      // Generate chunk groups
      for (int z = offset.y; z < offset.y + width; ++z) {
        for (int x = offset.x; x < offset.x + width; ++x) {
          Chunk *current = terrain.getFirstFlatChunk(glm::ivec2(x, z));

          while (current) {
            glm::ivec3 groupCoord = getChunkGroupCoord(
              terrain, current->chunkCoord);

            ChunkGroup *group = getChunkGroup(groupCoord);
            group->level = node->level;
            current->chunkGroupKey = group->key;

            int stride = (int)pow(2, terrain.mQuadTree.mMaxLOD - node->level);
            float width = stride;
            
            glm::vec3 chunkCoordOffset = (glm::vec3)(
              current->chunkCoord - groupCoord);

            glm::vec3 start = (float)CHUNK_DIM * (chunkCoordOffset / width);

            for (int z = 0 ; z < CHUNK_DIM / stride; ++z) {
              for (int y = 0 ; y < CHUNK_DIM / stride; ++y) {
                for (int x = 0 ; x < CHUNK_DIM / stride; ++x) {
                  glm::ivec3 coord = glm::ivec3(x, y, z);

                  uint32_t dstVIndex = getVoxelIndex(coord + (glm::ivec3)start);
                  uint32_t srcVIndex = getVoxelIndex(coord * stride);
                  group->voxels[dstVIndex] = current->voxels[srcVIndex];
                }
              }
            }

            if (current->next == INVALID_CHUNK_INDEX) {
              current = nullptr;
            }
            else {
              current = terrain.mLoadedChunks[current->next];
            }
          }
        }
      }
    }

    // Generate the vertices now
    uint32_t totalCount = 0;

    mNullChunkGroup = (ChunkGroup *)lnAlloc(sizeof(ChunkGroup));
    memset(mNullChunkGroup, 0, sizeof(ChunkGroup));

    // Later change this to just the updated chunks
    for (auto group : mChunkGroups) {
      Voxel surfaceDensity = {(uint16_t)30000};
      uint32_t vertexCount = generateVertices(
        terrain, *group, surfaceDensity, mTemporaryVertices);

      group->vertexCount = vertexCount;

      totalCount += vertexCount;

      if (group->verticesMemory.size()) {
        // This chunk already has allocated memory
        mGPUVerticesAllocator.free(group->verticesMemory);
      }

      if (vertexCount) {
        auto slot = mGPUVerticesAllocator.allocate(
          sizeof(ChunkVertex) * vertexCount);

        slot.write(
          commandBuffer,
          mTemporaryVertices,
          sizeof(ChunkVertex) * vertexCount);

        group->verticesMemory = slot;
      }
      else {
        group->verticesMemory = {};
      }
    }

    mGPUVerticesAllocator.debugLogState();
  }
}

uint32_t TerrainRenderer::generateVertices(
  const Terrain &terrain,
  const ChunkGroup &group,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices) {
  int groupSize = pow(2, terrain.mQuadTree.mMaxLOD - group.level);
  int stride = groupSize;
  // glm::ivec3 groupCoord = group.coord + glm::ivec3(
  // pow(2, terrain.mQuadTree.mMaxLOD - 1));
  glm::ivec3 groupStart = group.coord * (int)CHUNK_DIM;

  auto getVoxel = [&terrain, &groupSize, &groupStart](
    uint32_t x, uint32_t y, uint32_t z) {
    return terrain.getVoxel(groupStart + glm::ivec3(x, y, z) * groupSize);
  };

  auto getVoxelLOD = [&terrain, &groupSize, &groupStart, &stride](
    uint32_t x, uint32_t y, uint32_t z,
    // Offset
    uint32_t ox, uint32_t oy, uint32_t oz) {
    return terrain.getVoxel(
      groupStart + glm::ivec3(x, y, z) * groupSize +
      glm::ivec3(ox, oy, oz) * stride / 2);
  };

  uint32_t vertexCount = 0;

  for (uint32_t z = 1; z < CHUNK_DIM - 1; ++z) {
    for (uint32_t y = 1; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 1; x < CHUNK_DIM - 1; ++x) {
        Voxel voxelValues[8] = {
          group.voxels[getVoxelIndex(x,     y,     z)],
          group.voxels[getVoxelIndex(x + 1, y,     z)],
          group.voxels[getVoxelIndex(x,     y + 1, z)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z)],

          group.voxels[getVoxelIndex(x,     y,     z + 1)],
          group.voxels[getVoxelIndex(x + 1, y,     z + 1)],
          group.voxels[getVoxelIndex(x,     y + 1, z + 1)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z + 1)],
        };

        updateVoxelCell(
          voxelValues, glm::ivec3(x, y, z), surfaceDensity,
          meshVertices, vertexCount);
      }
    }
  }

  glm::ivec3 groupCoord = group.coord + glm::ivec3(
    pow(2, terrain.mQuadTree.mMaxLOD - 1));

#if 1
  updateChunkFace(
    terrain, group, surfaceDensity,
    0, 1, // Inner axis is X, outer axis is Y
    2, 1, // We are updating the positive Z face
    meshVertices, vertexCount);
#else
  glm::ivec3 posZ = groupCoord + glm::ivec3(0, 0, 1) * groupSize;
  QuadTree::NodeInfo posZNode = terrain.mQuadTree.getNodeInfo(
    glm::vec2(posZ.x, posZ.z));

  if (posZNode.level <= group.level) {
    uint32_t z = CHUNK_DIM - 1;
    for (uint32_t y = 1; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 1; x < CHUNK_DIM - 1; ++x) {

        Voxel voxelValues[8] = {
          group.voxels[getVoxelIndex(x,     y,     z)],
          group.voxels[getVoxelIndex(x + 1, y,     z)],
          group.voxels[getVoxelIndex(x,     y + 1, z)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z)],

          getVoxel(x,     y,     z + 1),
          getVoxel(x + 1, y,     z + 1),
          getVoxel(x,     y + 1, z + 1),
          getVoxel(x + 1, y + 1, z + 1)
        };

        updateVoxelCell(
          voxelValues, glm::ivec3(x, y, z), surfaceDensity,
          meshVertices, vertexCount);
      }
    }
  }
  else {
    uint32_t z = CHUNK_DIM - 1;
    for (uint32_t y = 1; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 1; x < CHUNK_DIM - 1; ++x) {
        Voxel voxelValues[8] = {
          group.voxels[getVoxelIndex(x,     y,     z)],
          group.voxels[getVoxelIndex(x + 1, y,     z)],
          group.voxels[getVoxelIndex(x,     y + 1, z)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z)],

          getVoxel(x,     y,     z + 1),
          getVoxel(x + 1, y,     z + 1),
          getVoxel(x,     y + 1, z + 1),
          getVoxel(x + 1, y + 1, z + 1)
        };

        Voxel transVoxels[13] = {
          getVoxelLOD(x,     y,     z + 1, 0, 0, 0),
          getVoxelLOD(x,     y,     z + 1, 1, 0, 0),
          getVoxelLOD(x + 1, y,     z + 1, 0, 0, 0),

          getVoxelLOD(x,     y,     z + 1, 0, 1, 0),
          getVoxelLOD(x,     y,     z + 1, 1, 1, 0),
          getVoxelLOD(x + 1, y,     z + 1, 0, 1, 0),

          getVoxelLOD(x,     y + 1, z + 1, 0, 0, 0),
          getVoxelLOD(x,     y + 1, z + 1, 1, 0, 0),
          getVoxelLOD(x + 1, y + 1, z + 1, 0, 0, 0),

          getVoxel(x,     y,     z + 1),
          getVoxel(x + 1, y,     z + 1),
          getVoxel(x,     y + 1, z + 1),
          getVoxel(x + 1, y + 1, z + 1)
        };

        for (int i = 0; i < 4; ++i) {
          transVoxels[i + 9].normalX = transVoxels[i + 9].normalX * 0.875 + voxelValues[i].normalX * 0.125;
          transVoxels[i + 9].normalY = transVoxels[i + 9].normalY * 0.875 + voxelValues[i].normalY * 0.125;
          transVoxels[i + 9].normalZ = transVoxels[i + 9].normalZ * 0.875 + voxelValues[i].normalZ * 0.125;
          voxelValues[i + 4].normalX = transVoxels[i + 9].normalX;
          voxelValues[i + 4].normalY = transVoxels[i + 9].normalY;
          voxelValues[i + 4].normalZ = transVoxels[i + 9].normalZ;
        }

        updateTransVoxelCell(
          voxelValues, transVoxels,
          glm::ivec3(0, 0, 1), glm::ivec3(x, y, z),
          surfaceDensity,
          meshVertices, vertexCount);
      }
    }
  }
#endif

  updateChunkFace(
    terrain, group, surfaceDensity,
    0, 1, // Inner axis is X, outer axis is Y
    2, 0, // We are updating the negative Z face
    meshVertices, vertexCount);

  return vertexCount;
}

void TerrainRenderer::updateChunkFace(
  const Terrain &terrain,
  const ChunkGroup &group,
  Voxel surfaceDensity,
  uint32_t primaryAxis, uint32_t secondAxis,
  uint32_t faceAxis, uint32_t side,
  ChunkVertex *meshVertices, uint32_t &vertexCount) {
  int groupSize = pow(2, terrain.mQuadTree.mMaxLOD - group.level);
  glm::ivec3 groupStart = group.coord * (int)CHUNK_DIM;
  int stride = groupSize;

  glm::ivec3 groupCoordOffset = glm::ivec3(0);
  groupCoordOffset[faceAxis] = (int)side * 2 - 1;
  glm::ivec3 adjacentCoord = group.coord + glm::ivec3(
    pow(2, terrain.mQuadTree.mMaxLOD - 1));
  if (side == 1) {
    adjacentCoord += groupCoordOffset * groupSize;
  }
  else {
    adjacentCoord += groupCoordOffset;
  }

  QuadTree::NodeInfo adjacentNode = terrain.mQuadTree.getNodeInfo(
    glm::vec2(adjacentCoord.x, adjacentCoord.z));

  if (adjacentNode.exists) {
    static const glm::ivec3 offsets[8] = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
      {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1},
    };

    auto getVoxel = [&terrain, &groupSize, &groupStart](
      uint32_t x, uint32_t y, uint32_t z) {
      return terrain.getVoxel(groupStart + glm::ivec3(x, y, z) * groupSize);
    };

    auto getVoxelLOD = [&terrain, &groupSize, &groupStart, &stride](
      uint32_t x, uint32_t y, uint32_t z,
      // Offset
      uint32_t ox, uint32_t oy, uint32_t oz) {
      return terrain.getVoxel(
        groupStart + glm::ivec3(x, y, z) * groupSize +
        glm::ivec3(ox, oy, oz) * stride / 2);
    };

    if (adjacentNode.level <= group.level) {
      uint32_t d2 = (CHUNK_DIM - 1) * side;
      for (uint32_t d1 = 1; d1 < CHUNK_DIM - 1; ++d1) {
        for (uint32_t d0 = 1; d0 < CHUNK_DIM - 1; ++d0) {
          Voxel voxelValues[8] = {};

          for (int i = 0; i < 8; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i].x;
            voxelCoord[secondAxis] = d1 + offsets[i].y;
            voxelCoord[faceAxis] = d2 + offsets[i].z;
          
            voxelValues[i] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          glm::ivec3 coord = {};
          coord[primaryAxis] = d0;
          coord[secondAxis] = d1;
          coord[faceAxis] = d2;

          updateVoxelCell(
            voxelValues, coord, surfaceDensity,
            meshVertices, vertexCount);
        }
      }
    }
    else {
      static const glm::ivec4 transOffsets[9] = {
        {0,0,0,0}, {0,0,1,0}, {1,0,0,0}, {0,0,0,1}, {0,0,1,1}, {1,0,0,1},
        {0,1,0,0}, {0,1,1,0}, {1,1,0,0}
      };

      uint32_t d2 = (CHUNK_DIM - 1) * side;
      for (uint32_t d1 = 1; d1 < CHUNK_DIM - 1; ++d1) {
        for (uint32_t d0 = 1; d0 < CHUNK_DIM - 1; ++d0) {
          Voxel voxelValues[8] = {};
          for (int i = 0; i < 8; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i][0];
            voxelCoord[secondAxis] = d1 + offsets[i][1];
            voxelCoord[faceAxis] = d2 + offsets[i][2];

            voxelValues[i] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          Voxel transVoxels[13] = {};

          for (int i = 0; i < 9; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + transOffsets[i][0];
            voxelCoord[secondAxis] = d1 + transOffsets[i][1];
            voxelCoord[faceAxis] = d2 + side;

            glm::ivec3 halfCoord = {};
            halfCoord[primaryAxis] = transOffsets[i][2];
            halfCoord[secondAxis] = transOffsets[i][3];

            transVoxels[i] = getVoxelLOD(
              voxelCoord.x, voxelCoord.y, voxelCoord.z,
              halfCoord.x, halfCoord.y, halfCoord.z);
          }

          for (int i = 0; i < 4; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i][0];
            voxelCoord[secondAxis] = d1 + offsets[i][1];
            voxelCoord[faceAxis] = d2 + side;

            transVoxels[i + 9] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          for (int i = 0; i < 4; ++i) {
            transVoxels[i + 9].normalX = transVoxels[i + 9].normalX * 0.875 + voxelValues[i + 4 * (1 - side)].normalX * 0.125;
            transVoxels[i + 9].normalY = transVoxels[i + 9].normalY * 0.875 + voxelValues[i + 4 * (1 - side)].normalY * 0.125;
            transVoxels[i + 9].normalZ = transVoxels[i + 9].normalZ * 0.875 + voxelValues[i + 4 * (1 - side)].normalZ * 0.125;
            voxelValues[i + 4 * side].normalX = transVoxels[i + 9].normalX;
            voxelValues[i + 4 * side].normalY = transVoxels[i + 9].normalY;
            voxelValues[i + 4 * side].normalZ = transVoxels[i + 9].normalZ;
          }

          glm::ivec3 coord = {};
          coord[primaryAxis] = d0;
          coord[secondAxis] = d1;
          coord[faceAxis] = d2;

          glm::ivec3 axis = {};
          axis[faceAxis] = side * 2 - 1;

          updateTransVoxelCell(
            voxelValues, transVoxels,
            axis, coord,
            surfaceDensity,
            meshVertices, vertexCount);
        }
      }
    }
  }
}

#include "Transvoxel.inc"

void TerrainRenderer::pushVertexToTriangleList(
  uint32_t v0, uint32_t v1,
  glm::vec3 *vertices, Voxel *voxels,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices, uint32_t &vertexCount) {
  float surfaceLevelF = (float)surfaceDensity.density;
  float voxelValue0 = (float)voxels[v0].density;
  float voxelValue1 = (float)voxels[v1].density;

  if (voxelValue0 > voxelValue1) {
    float tmp = voxelValue0;
    voxelValue0 = voxelValue1;
    voxelValue1 = tmp;

    uint8_t tmpV = v0;
    v0 = v1;
    v1 = tmpV;
  }

  float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
  glm::vec3 vertex = interpolate(
    vertices[v0], vertices[v1], interpolatedVoxelValues);

  glm::vec3 normal0 = glm::vec3(
    voxels[v0].normalX, voxels[v0].normalY, voxels[v0].normalZ) / 1000.0f;
  glm::vec3 normal1 = glm::vec3(
    voxels[v1].normalX, voxels[v1].normalY, voxels[v1].normalZ) / 1000.0f;

  glm::vec3 normal = interpolate(
    normal0, normal1, interpolatedVoxelValues);

  meshVertices[vertexCount] = {vertex, normal};

  ++vertexCount;
}

void TerrainRenderer::updateVoxelCell(
  Voxel *voxels,
  const glm::ivec3 &coord,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices,
  uint32_t &vertexCount) {
  uint8_t bitCombination = 0;
  for (uint32_t i = 0; i < 8; ++i) {
    bool isOverSurface = (voxels[i].density > surfaceDensity.density);
    bitCombination |= isOverSurface << i;
  }

  if (bitCombination == 0 || bitCombination == 0xFF) {
    return;
  }

  uint8_t cellClassIdx = regularCellClass[bitCombination];
  const RegularCellData &cellData = regularCellData[cellClassIdx];

  glm::vec3 vertices[8] = {};
  for (uint32_t i = 0; i < 8; ++i) {
    vertices[i] = NORMALIZED_CUBE_VERTICES[i] +
      glm::vec3(0.5f) + glm::vec3(coord);
  }

  ChunkVertex *verts = STACK_ALLOC(ChunkVertex, cellData.GetVertexCount());

  for (int i = 0; i < cellData.GetVertexCount(); ++i) {
    uint16_t nibbles = regularVertexData[bitCombination][i];

    if (nibbles == 0x0) {
      // Finished
      break;
    }

    uint8_t v0 = (nibbles >> 4) & 0xF;
    uint8_t v1 = nibbles & 0xF;

    float surfaceLevelF = (float)surfaceDensity.density;
    float voxelValue0 = (float)voxels[v0].density;
    float voxelValue1 = (float)voxels[v1].density;

    if (voxelValue0 > voxelValue1) {
      float tmp = voxelValue0;
      voxelValue0 = voxelValue1;
      voxelValue1 = tmp;

      uint8_t tmpV = v0;
      v0 = v1;
      v1 = tmpV;
    }

    float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
    glm::vec3 vertex = interpolate(
      vertices[v0], vertices[v1], interpolatedVoxelValues);

    glm::vec3 normal0 = glm::vec3(
      voxels[v0].normalX, voxels[v0].normalY, voxels[v0].normalZ) / 1000.0f;
    glm::vec3 normal1 = glm::vec3(
      voxels[v1].normalX, voxels[v1].normalY, voxels[v1].normalZ) / 1000.0f;

    glm::vec3 normal = interpolate(
      normal0, normal1, interpolatedVoxelValues);

    verts[i] = {vertex, normal};
  }

  for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
    int vertexIndex = cellData.vertexIndex[i];
    meshVertices[vertexCount++] = verts[vertexIndex];
  }
}

void TerrainRenderer::updateTransVoxelCell(
  Voxel *voxels,
  Voxel *transVoxels,
  const glm::ivec3 &axis,
  const glm::ivec3 &coord,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices,
  uint32_t &vertexCount) {
  const float percentTrans = 0.125f;

  { // Normal mesh creation
    uint8_t bitCombination = 0;
    for (uint32_t i = 0; i < 8; ++i) {
      bool isOverSurface = (voxels[i].density > surfaceDensity.density);
      bitCombination |= isOverSurface << i;
    }

    if (bitCombination == 0 || bitCombination == 0xFF) {
      return;
    }

    uint8_t cellClassIdx = regularCellClass[bitCombination];
    const RegularCellData &cellData = regularCellData[cellClassIdx];

    glm::vec3 vertices[8] = {};
    for (uint32_t i = 0; i < 8; ++i) {
      vertices[i] = NORMALIZED_CUBE_VERTICES[i] +
        glm::vec3(0.5f) + glm::vec3(coord);
    }

    if (axis.z == -1) {
      vertices[0].z += percentTrans;
      vertices[1].z += percentTrans;
      vertices[2].z += percentTrans;
      vertices[3].z += percentTrans;
    }
    else if (axis.z == 1) {
      vertices[4].z -= percentTrans;
      vertices[5].z -= percentTrans;
      vertices[6].z -= percentTrans;
      vertices[7].z -= percentTrans;
    }

    ChunkVertex *verts = STACK_ALLOC(ChunkVertex, cellData.GetVertexCount());

    for (int i = 0; i < cellData.GetVertexCount(); ++i) {
      uint16_t nibbles = regularVertexData[bitCombination][i];

      if (nibbles == 0x0) {
        // Finished
        break;
      }

      uint8_t v0 = (nibbles >> 4) & 0xF;
      uint8_t v1 = nibbles & 0xF;

      float surfaceLevelF = (float)surfaceDensity.density;
      float voxelValue0 = (float)voxels[v0].density;
      float voxelValue1 = (float)voxels[v1].density;

      if (voxelValue0 > voxelValue1) {
        float tmp = voxelValue0;
        voxelValue0 = voxelValue1;
        voxelValue1 = tmp;

        uint8_t tmpV = v0;
        v0 = v1;
        v1 = tmpV;
      }

      float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
      glm::vec3 vertex = interpolate(
        vertices[v0], vertices[v1], interpolatedVoxelValues);

      glm::vec3 normal0 = glm::vec3(
        voxels[v0].normalX, voxels[v0].normalY, voxels[v0].normalZ) / 1000.0f;
      glm::vec3 normal1 = glm::vec3(
        voxels[v1].normalX, voxels[v1].normalY, voxels[v1].normalZ) / 1000.0f;

      glm::vec3 normal = interpolate(
        normal0, normal1, interpolatedVoxelValues);

      verts[i] = {vertex, normal};
    }

    for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
      int vertexIndex = cellData.vertexIndex[i];
      meshVertices[vertexCount++] = verts[vertexIndex];
    }
  }

  if (1) { // Transvoxel mesh creation
    uint32_t bitCombination = 0;

    bitCombination |= (transVoxels[0].density > surfaceDensity.density) << 0;
    bitCombination |= (transVoxels[1].density > surfaceDensity.density) << 1;
    bitCombination |= (transVoxels[2].density > surfaceDensity.density) << 2;
    bitCombination |= (transVoxels[3].density > surfaceDensity.density) << 7;
    bitCombination |= (transVoxels[4].density > surfaceDensity.density) << 8;
    bitCombination |= (transVoxels[5].density > surfaceDensity.density) << 3;
    bitCombination |= (transVoxels[6].density > surfaceDensity.density) << 6;
    bitCombination |= (transVoxels[7].density > surfaceDensity.density) << 5;
    bitCombination |= (transVoxels[8].density > surfaceDensity.density) << 4;

    if (bitCombination == 0 || bitCombination == 511) {
      return;
    }

    int detailed0, detailed1, nonDetailed, nonDetailedBit;
    if (axis.z == -1) {
      detailed0 = 0, detailed1 = 1, nonDetailed = 2, nonDetailedBit = 0;
    }
    else if (axis.z == 1) {
      detailed0 = 0, detailed1 = 1, nonDetailed = 2, nonDetailedBit = 1;
    }

    glm::vec3 vertices[13] = {};
    uint32_t counter = 0;

    for (int d1 = 0; d1 < 3; ++d1) {
      for (int d0 = 0; d0 < 3; ++d0) {
        vertices[counter][detailed0] = (float)d0 * 0.5f;
        vertices[counter][detailed1] = (float)d1 * 0.5f;
        vertices[counter][nonDetailed] = (float)nonDetailedBit;

        vertices[counter] += glm::vec3(coord);
        ++counter;
      }
    }

    for (int d1 = 0; d1 < 2; ++d1) {
      for (int d0 = 0; d0 < 2; ++d0) {
        vertices[counter][detailed0] = (float)d0;
        vertices[counter][detailed1] = (float)d1;
        vertices[counter][nonDetailed] = (float)(nonDetailedBit ^ 1);

        vertices[counter] += glm::vec3(coord);
        ++counter;
      }
    }

    if (nonDetailedBit) {
      if (axis[nonDetailed] == 1) {
        for (int i = 9; i < 13; ++i) {
          vertices[i][nonDetailed] += (1.0f - percentTrans);
        }
      }
    }
    else {
      if (axis[nonDetailed] == -1) {
        for (int i = 9; i < 13; ++i) {
          vertices[i][nonDetailed] -= (1.0f - percentTrans);
        }
      }
    }

    uint8_t cellClassIdx = transitionCellClass[bitCombination];
    const TransitionCellData &cellData = transitionCellData[cellClassIdx & 0x7F];

    ChunkVertex *verts = STACK_ALLOC(ChunkVertex, cellData.GetVertexCount());

    for (int i = 0; i < cellData.GetVertexCount(); ++i) {
      uint16_t nibbles = transitionVertexData[bitCombination][i];

      if (nibbles == 0x0) {
        break;
      }

      uint8_t v0 = (nibbles >> 4) & 0xF;
      uint8_t v1 = nibbles & 0xF;

      float surfaceLevelF = (float)surfaceDensity.density;
      float voxelValue0 = (float)transVoxels[v0].density;
      float voxelValue1 = (float)transVoxels[v1].density;

      if (voxelValue0 > voxelValue1) {
        float tmp = voxelValue0;
        voxelValue0 = voxelValue1;
        voxelValue1 = tmp;

        uint8_t tmpV = v0;
        v0 = v1;
        v1 = tmpV;
      }

      float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
      glm::vec3 vertex = interpolate(
        vertices[v0], vertices[v1], interpolatedVoxelValues);

      glm::vec3 normal0 = glm::vec3(
        transVoxels[v0].normalX,
        transVoxels[v0].normalY,
        transVoxels[v0].normalZ) / 1000.0f;

      glm::vec3 normal1 = glm::vec3(
        transVoxels[v1].normalX,
        transVoxels[v1].normalY,
        transVoxels[v1].normalZ) / 1000.0f;

      glm::vec3 normal = interpolate(
        normal0, normal1, interpolatedVoxelValues);

      glm::vec3 diff = vertex - (glm::vec3)coord;

      if (diff.x < 0.0f || diff.x > 1.0f ||
          diff.y < 0.0f || diff.y > 1.0f ||
          diff.z < 0.0f || diff.z > 1.0f) {
        printf("BUG\n");
      }

      verts[i] = {vertex, normal};
    }

    if (cellClassIdx & 0x80) {
      for (int i = cellData.GetTriangleCount() * 3 - 1; i >= 0; --i) {
        int vertexIndex = cellData.vertexIndex[i];

        glm::vec3 diff = verts[vertexIndex].position - (glm::vec3)coord;

        if (diff.x < 0.0f || diff.x > 1.0f ||
            diff.y < 0.0f || diff.y > 1.0f ||
            diff.z < 0.0f || diff.z > 1.0f) {
          printf("BUG\n");
        }

        meshVertices[vertexCount++] = verts[vertexIndex];
      }
    }
    else {
      for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
        int vertexIndex = cellData.vertexIndex[i];

        glm::vec3 diff = verts[vertexIndex].position - (glm::vec3)coord;

        if (diff.x < 0.0f || diff.x > 1.0f ||
            diff.y < 0.0f || diff.y > 1.0f ||
            diff.z < 0.0f || diff.z > 1.0f) {
          printf("BUG\n");
        }

        meshVertices[vertexCount++] = verts[vertexIndex];
      }
    }
  }
}

uint32_t TerrainRenderer::hashChunkGroupCoord(const glm::ivec3 &coord) const {
  struct {
    union {
      struct {
        uint32_t padding: 2;
        uint32_t x: 10;
        uint32_t y: 10;
        uint32_t z: 10;
      };
      uint32_t value;
    };
  } hasher;

  hasher.value = 0;

  hasher.x = *(uint32_t *)(&coord.x);
  hasher.y = *(uint32_t *)(&coord.y);
  hasher.z = *(uint32_t *)(&coord.z);

  return (uint32_t)hasher.value;
}

glm::ivec3 TerrainRenderer::getChunkGroupCoord(
  const Terrain &terrain,
  const glm::ivec3 &chunkCoord) const {
  glm::ivec2 quadTreeCoord = glm::ivec2(chunkCoord.x, chunkCoord.z) +
    glm::ivec2(glm::pow(2, terrain.mQuadTree.maxLOD() - 1));

  QuadTree::NodeInfo node = terrain.mQuadTree.getNodeInfo(quadTreeCoord);

  node.offset -= glm::vec2(glm::pow(2.0f, terrain.mQuadTree.maxLOD() - 1));
  glm::ivec3 coord = glm::ivec3(node.offset.x, chunkCoord.y, node.offset.y);
  // Round down the nearest 2^node.level
  coord.y -= coord.y % (int)pow(2, node.level);

  return coord;
}

ChunkGroup *TerrainRenderer::getChunkGroup(const glm::ivec3 &coord) {
  uint32_t hash = hashChunkGroupCoord(coord);
  uint32_t *index = mChunkGroupIndices.get(hash);
    
  if (index) {
    // Chunk was already added
    return mChunkGroups[*index];
  }
  else {
    ChunkGroup *group = flAlloc<ChunkGroup>();

    auto key = mChunkGroups.add(group);

    zeroMemory(group);
    group->coord = coord;
    group->key = key;

    mChunkGroupIndices.insert(hash, key);

    return group;
  }
}

}
