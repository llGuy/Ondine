#include <iostream>
#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "Application.hpp"
#include "RendererCache.hpp"
#include "RendererDebug.hpp"
#include "VulkanRenderPass.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext) {
  
}

void Renderer3D::init() {
  auto properties = mGraphicsContext.getProperties();
  pipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };

  mModelManager.init();
  mRenderMethods.init();
  mShaderEntries.init();
  mGBuffer.init(
    mGraphicsContext, {pipelineViewport.width, pipelineViewport.height});
  mSkyRenderer.init(mGraphicsContext, mGBuffer);
  mStarRenderer.init(mGraphicsContext, mGBuffer, 1000);
  mTerrainRenderer.init(mGraphicsContext, mGBuffer);

  // Idle with all precomputation stuff
  mGraphicsContext.device().graphicsQueue().idle();

  { // Set planet properties
    mPlanetProperties.solarIrradiance = glm::vec3(1.474f, 1.8504f, 1.91198f);

    // Angular radius of the Sun (radians)
    mPlanetProperties.solarAngularRadius = 0.004695f;
    mPlanetProperties.bottomRadius = 6360.0f / 6.0f;
    mPlanetProperties.topRadius = 6420.0f / 6.0f;

    mPlanetProperties.rayleighDensity.layers[0] =
      DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    mPlanetProperties.rayleighDensity.layers[1] =
      DensityLayer { 0.0f, 1.0f, -0.125f, 0.0f, 0.0f };
    mPlanetProperties.rayleighScatteringCoef =
      glm::vec3(0.005802f, 0.013558f, 0.033100f);

    mPlanetProperties.mieDensity.layers[0] =
      DensityLayer { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    mPlanetProperties.mieDensity.layers[1] =
      DensityLayer { 0.0f, 1.0f, -0.833333f, 0.0f, 0.0f };
    mPlanetProperties.mieScatteringCoef = glm::vec3(
      0.003996f, 0.003996f, 0.003996f);
    mPlanetProperties.mieExtinctionCoef = glm::vec3(
      0.004440f, 0.004440f, 0.004440f);

    mPlanetProperties.miePhaseFunctionG = 0.8f;

    mPlanetProperties.absorptionDensity.layers[0] =
      DensityLayer { 25.000000f, 0.000000f, 0.000000f, 0.066667f, -0.666667f };
    mPlanetProperties.absorptionDensity.layers[1] =
      DensityLayer { 0.000000f, 0.000000f, 0.000000f, -0.066667f, 2.666667f };
    mPlanetProperties.absorptionExtinctionCoef =
      glm::vec3(0.000650f, 0.001881f, 0.000085f);
    mPlanetProperties.groundAlbedo = glm::vec3(0.05f, 0.05f, 0.05f);
    mPlanetProperties.muSunMin = -0.207912f;
    mPlanetProperties.wPlanetCenter =
      glm::vec3(0.0f, -mPlanetProperties.bottomRadius, 0.0f);

    mPlanetRenderer.init(mGraphicsContext, mGBuffer, &mPlanetProperties);
  }

  mCamera.init(mGraphicsContext);
  mDeferredLighting.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height});
  mClipping.init(mGraphicsContext, 0.0f, mPlanetProperties.bottomRadius);

  { // Prepare scene resources
    /* Create model */
    ModelConfig modelConfig;
    auto sphereModelHandle = mModelManager.loadStaticModel(
      "res/model/UVSphere.fbx", mGraphicsContext, modelConfig);
    auto monkeyModelHandle = mModelManager.loadStaticModel(
      "res/model/Taurus.fbx", mGraphicsContext, modelConfig);
    
    /* Create shader */
    VulkanPipelineConfig pipelineConfig(
      {mGBuffer.renderPass(), 0},
      VulkanShader{mGraphicsContext.device(), "res/spv/BaseModel.vert.spv"},
      VulkanShader{mGraphicsContext.device(), "res/spv/BaseModel.frag.spv"});

    pipelineConfig.enableDepthTesting();
    pipelineConfig.configurePipelineLayout(
      sizeof(glm::mat4),
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

    modelConfig.configureVertexInput(pipelineConfig);
    pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    auto &baseModelShader = mShaderEntries.emplace("BaseModelShader");
      
    baseModelShader.init(
      mGraphicsContext.device(),
      mGraphicsContext.descriptorLayouts(),
      pipelineConfig);

    /* Create render method */
    RenderMethod baseModelMethod(mModelManager, mShaderEntries);
    baseModelMethod.init(
      "BaseModelShader", sphereModelHandle,
      [](const VulkanCommandBuffer &cmdbuf, const RenderResources &res) {
        cmdbuf.bindUniforms(
          res.camera.uniform(), res.planet.uniform(), res.clipping.uniform);
      },
      [](const VulkanCommandBuffer &cmdbuf, const SceneObject &obj) {
        cmdbuf.pushConstants(sizeof(glm::mat4), &obj.transform);
      });

    mRenderMethods.insert("SphereModelRenderMethod", baseModelMethod);

    RenderMethod monkeyModelMethod(mModelManager, mShaderEntries);
    monkeyModelMethod.init(
      "BaseModelShader", monkeyModelHandle,
      [](const VulkanCommandBuffer &cmdbuf, const RenderResources &res) {
        cmdbuf.bindUniforms(
          res.camera.uniform(), res.planet.uniform(), res.clipping.uniform);
      },
      [](const VulkanCommandBuffer &cmdbuf, const SceneObject &obj) {
        cmdbuf.pushConstants(sizeof(glm::mat4), &obj.transform);
      });

    mRenderMethods.insert("TaurusModelRenderMethod", monkeyModelMethod);
  }

  mWaterRenderer.init(mGraphicsContext, mPlanetProperties);

  mPixelater.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height});

  // Testing vulkan arena allocator
  mArena.init(
    20,
    (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer,
    mGraphicsContext);

#if 0
  VulkanArenaSlot slot0 = mArena.allocate(0x1000);
  VulkanArenaSlot slot1 = mArena.allocate(0x2000);
  VulkanArenaSlot slot2 = mArena.allocate(0x3000);
  VulkanArenaSlot slot3 = mArena.allocate(0x4000);
  VulkanArenaSlot slot4 = mArena.allocate(0x5000);
  mArena.debugLogState();

  std::cin.get();

  mArena.free(slot1.offset);
  mArena.debugLogState();

  std::cin.get();

  mArena.free(slot0.offset);
  mArena.debugLogState();

  std::cin.get();

  mArena.free(slot3.offset);
  mArena.debugLogState();

  std::cin.get();

  slot1 = mArena.allocate(0x2000);
  mArena.debugLogState();

  std::cin.get();

  slot3 = mArena.allocate(0x4000);
  mArena.debugLogState();

  LOG_INFO("Last before game\n");
  std::cin.get();
#endif
}

void Renderer3D::shutdown() {
  if (!Core::gFileSystem->isPathValid(
        (Core::MountPoint)Core::ApplicationMountPoints::Application,
        DRAW_CACHE_DIRECTORY)) {
    Core::gFileSystem->makeDirectory(
      (Core::MountPoint)Core::ApplicationMountPoints::Application,
      DRAW_CACHE_DIRECTORY);
  }

  mSkyRenderer.shutdown(mGraphicsContext);
}

void Renderer3D::tick(const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  mTerrainRenderer.sync(mBoundScene->terrain, frame.primaryCommandBuffer);

  mBoundScene->lighting.tick(tick, mPlanetProperties);
  mBoundScene->camera.tick(
    {mGBuffer.mGBufferExtent.width, mGBuffer.mGBufferExtent.height});

  mCamera.updateData(frame.primaryCommandBuffer, mBoundScene->camera);

  mDeferredLighting.updateData(
    frame.primaryCommandBuffer, mBoundScene->lighting);

  mStarRenderer.tick(
    mBoundScene->camera,
    mBoundScene->lighting,
    mPlanetProperties, tick);

  mWaterRenderer.updateCameraInfo(mBoundScene->camera, mPlanetProperties);
  mWaterRenderer.updateCameraUBO(frame.primaryCommandBuffer);
  mWaterRenderer.updateLightingUBO(
    mBoundScene->lighting, frame.primaryCommandBuffer);

  /* Rendering to water texture */
  mWaterRenderer.tick(
    frame, mPlanetRenderer, mSkyRenderer,
    mStarRenderer, mTerrainRenderer, *mBoundScene);
     
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    mBoundScene->submit(
      mCamera, mPlanetRenderer, mClipping, mTerrainRenderer, frame);
    mBoundScene->submitDebug(
      mCamera, mPlanetRenderer, mClipping, mTerrainRenderer, frame);
    mStarRenderer.render(3.0f, mCamera, frame);
  }
  mGBuffer.endRender(frame);

  mDeferredLighting.render(
    frame, mGBuffer, mCamera, mPlanetRenderer, mWaterRenderer, mSkyRenderer);

  mPixelater.render(frame, mDeferredLighting);
}

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  pipelineViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(mGraphicsContext, newResolution);
  mDeferredLighting.resize(mGraphicsContext, newResolution);

  mBoundScene->camera.mAspectRatio =
    (float)pipelineViewport.width / (float)pipelineViewport.height;

  mWaterRenderer.resize(mGraphicsContext, newResolution);

  mPixelater.resize(mGraphicsContext, newResolution);
}

void Renderer3D::trackPath(Core::TrackPathID id, const char *path) {
  mGraphicsContext.device().idle();

  ResourceTracker *trackers[] = {
    &mPixelater,
    &mDeferredLighting
  };

  // Add other file trackers after
  for (int i = 0; i < sizeof(trackers) / sizeof(trackers[0]); ++i) {
    trackers[i]->trackPath(mGraphicsContext, id, path);
  }
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mPixelater;
}

Scene *Renderer3D::createScene() {
  Scene *ret = new Scene(mModelManager, mRenderMethods);
  ret->init(mGBuffer, mGraphicsContext);
  return ret;
}

void Renderer3D::bindScene(Scene *scene) {
  mBoundScene = scene;
}

}
