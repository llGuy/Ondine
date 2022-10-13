#pragma once

#include "Chunk.hpp"
#include <glm/glm.hpp>
#include "FastMap.hpp"
#include "QuadTree.hpp"
#include "NumericMap.hpp"
#include "ThreadPool.hpp"
#include "Isosurface.hpp"
#include "VulkanPipeline.hpp"

namespace Ondine::View {

class EditorView;
class MapView;

}

namespace Ondine::Graphics {

class GBuffer;
class Camera;
class PlanetRenderer;
class Clipping;
class Terrain;
class VulkanContext;
struct VulkanFrame;
struct CameraProperties;

class TerrainRenderer {
public:
  void init(
    VulkanContext &graphicsContext,
    const GBuffer &gbuffer);

  // Temporary
  void setUpdateQuadTree(bool shouldUpdate);

  void render(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderWireframe(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderChunkOutlines(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void renderQuadTree(
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

  void sync(
    Terrain &terrain,
    const CameraProperties &camera,
    const VulkanCommandBuffer &commandBuffer);

  void forceFullUpdate();

private:
  void updateChunkGroupsSnapshots(const VulkanCommandBuffer &commandBuffer);

  struct GenerateMeshParams {
    TerrainRenderer *terrainRenderer;
    QuadTree *quadTree;
    Terrain *terrain;
  };

  static int runIsosurfaceExtraction(void *data);

private:
  void renderQuadTreeNode(
    QuadTree::Node *node,
    const glm::ivec2 &offset,
    const Camera &camera,
    const PlanetRenderer &planet,
    const Clipping &clipping,
    Terrain &terrain,
    VulkanFrame &frame);

private:
  VulkanPipeline mPipeline;
  VulkanPipeline mPipelineWireframe;
  // For debugging purposes
  VulkanPipeline mRenderLine;

  Isosurface mIsosurface;
  Stack<IsoGroupSnapshot> mSnapshots;

  QuadTree mQuadTree;
  Core::JobID mGenerationJob;

  GenerateMeshParams *mParams;

  // Temporary
  bool mUpdateQuadTree;
  bool mIsWaitingForSnapshots;

  friend class View::EditorView;
  friend class View::MapView;
};

}
