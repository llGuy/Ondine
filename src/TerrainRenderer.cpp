#include "Math.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include "TerrainRenderer.hpp"
#include <glm/gtx/string_cast.hpp>

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

  pipelineConfig.setToWireframe();

  mPipelineWireframe.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);

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

  mSnapshots.init(1000);

  mQuadTree.init(2);

  mIsosurface.init(mQuadTree, graphicsContext);

  mGenerationJob = Core::gThreadPool->createJob(runIsosurfaceExtraction);

  mParams = new GenerateMeshParams;
}

void TerrainRenderer::queueQuadTreeUpdate() {
  mUpdateQuadTree = true;
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

  for (auto group : mSnapshots) {
    float lodScale = glm::pow(2.0f, mQuadTree.mMaxLOD - group.level);
    glm::vec3 scale = glm::vec3(terrain.mTerrainScale) * lodScale;
    glm::vec3 tran = (glm::vec3)terrain.chunkCoordToWorld(group.coord) *
      (float)terrain.mTerrainScale;

    glm::mat4 translate = glm::translate(tran) * glm::scale(scale);

    if (group.vertices.size()) {
      commandBuffer.bindVertexBuffersArena(group.vertices);
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group.vertexCount, 1, 0, 0);
    }

    if (group.transVertices.size()) {
      commandBuffer.bindVertexBuffersArena(group.transVertices);
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group.transVertexCount, 1, 0, 0);
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

  for (auto group : mSnapshots) {
    float lodScale = glm::pow(2.0f, mQuadTree.mMaxLOD - group.level);
    glm::vec3 scale = glm::vec3(terrain.mTerrainScale) * lodScale;
    glm::vec3 tran = (glm::vec3)terrain.chunkCoordToWorld(group.coord) *
      (float)terrain.mTerrainScale;

    glm::mat4 translate = glm::translate(tran) * glm::scale(scale);

    if (group.vertices.size()) {
      commandBuffer.bindVertexBuffersArena(group.vertices);
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group.vertexCount, 1, 0, 0);
    }

    if (group.transVertices.size()) {
      commandBuffer.bindVertexBuffersArena(group.transVertices);
      commandBuffer.pushConstants(sizeof(translate), &translate[0][0]);
      commandBuffer.draw(group.transVertexCount, 1, 0, 0);
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

  for (auto group : mSnapshots) {
    if (group.vertices.size()) {
      float scale = (float)terrain.mTerrainScale * (float)CHUNK_DIM;
        
      pushConstant.transform = glm::vec4(
        (float)group.coord.x,
        (float)group.coord.y,
        (float)group.coord.z,
        scale);

      float stretch = glm::pow(2.0f, mQuadTree.mMaxLOD - group.level);

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

  for (int i = 0; i < mQuadTree.mDeepestNodes.size; ++i) {
    QuadTree::Node *node = mQuadTree.mDeepestNodes[i];

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
    glm::ivec2 wOffset = mIsosurface.quadTreeCoordsToWorld(
      mQuadTree,
      glm::ivec2(node->offsetx, node->offsety));
 
    float width = (float)(pow(2, mQuadTree.mMaxLOD - node->level) *
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
  glm::ivec2 wOffset = mIsosurface.quadTreeCoordsToWorld(
    mQuadTree,
    glm::ivec2(node->offsetx, node->offsety));
 
  float width = (float)(pow(2, mQuadTree.mMaxLOD - node->level) *
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

  if (node->level < mQuadTree.mMaxLOD) {
    int widthUnder = pow(2, mQuadTree.mMaxLOD - node->level - 1);
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
  const CameraProperties &camera,
  const VulkanCommandBuffer &commandBuffer) {
  mIsosurface.bindToTerrain(terrain);

  glm::vec2 pos = mIsosurface.worldToQuadTreeCoords(
    mQuadTree,
    glm::vec2(camera.wPosition.x, camera.wPosition.z));

  if (Core::gThreadPool->isJobFinished(mGenerationJob)) {
    /*
       If we were waiting on a new snapshot of chunk groups, we need to make
       sure to update the snapshots.
    */
    if (mIsWaitingForSnapshots) {
      mIsWaitingForSnapshots = false;
      LOG_INFO("Got snapshots\n");
      updateChunkGroupsSnapshots(commandBuffer);
    }

    /* 
       If the job has been finished, and we need to update the quad tree
       queue the job again.
    */
    /* if (mUpdateQuadTree) */ {
      mQuadTree.setFocalPoint(pos);

      if (mQuadTree.mDiffAdd.size()) {
        mUpdateQuadTree = false;
        mIsWaitingForSnapshots = true;
        mParams->terrainRenderer = this;
        mParams->terrain = &terrain;
        mParams->quadTree = &mQuadTree;
        Core::gThreadPool->startJob(mGenerationJob, mParams);
      }
    }
  }
}

void TerrainRenderer::updateChunkGroupsSnapshots(
  const VulkanCommandBuffer &commandBuffer) {
  mIsosurface.syncWithGPU(commandBuffer);

  mSnapshots.clear();
  for (auto group : mIsosurface.isoGroups()) {
    mSnapshots.push({
        group->coord, group->level,
        group->vertexCount, group->transVoxelVertexCount,
        group->vertices, group->transVoxelVertices
      });
  }
}

int TerrainRenderer::runIsosurfaceExtraction(void *data) {
  auto *params = (GenerateMeshParams *)data;
  TerrainRenderer *terrainRenderer = params->terrainRenderer;
  QuadTree *quadTree = params->quadTree;
  Terrain *terrain = params->terrain;

  terrainRenderer->mIsosurface.prepareForUpdate(*quadTree, *terrain);

  return 0;
}

}
