#include <iostream>
#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "FileSystem.hpp"
#include "RendererCache.hpp"
#include "RendererDebug.hpp"
#include "AssimpImporter.hpp"
#include "VulkanRenderPass.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

Renderer3D::Renderer3D(VulkanContext &graphicsContext)
  : mGraphicsContext(graphicsContext) {
  
}

void Renderer3D::init() {
  if (!gAssimpImporter) {
    gAssimpImporter = flAlloc<Assimp::Importer>();
  }

  auto properties = mGraphicsContext.getProperties();
  pipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };

  mModelManager.init();
  mRenderMethods.init();
  mShaderEntries.init();

  mCamera.init(mGraphicsContext);

  mToneMapping.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height},
    {glm::vec4(2.0f, 2.0f, 2.0f, 1.0f), 20.0f});

  mGraphicsContext.device().idle();
}

void Renderer3D::shutdown() {

}

void Renderer3D::tick(const Core::Tick &tick, Graphics::VulkanFrame &frame) {
  mToneMapping.render(frame);
}

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  pipelineViewport = {
    newResolution.width, newResolution.height
  };

#if 0
  mGBuffer.resize(mGraphicsContext, newResolution);
  mDeferredLighting.resize(mGraphicsContext, newResolution);

  mBoundScene->camera.mAspectRatio =
    (float)pipelineViewport.width / (float)pipelineViewport.height;

  mWaterRenderer.resize(mGraphicsContext, newResolution);

  mPixelater.resize(mGraphicsContext, newResolution);

  mBloomRenderer.resize(mGraphicsContext, newResolution);

#endif
  mToneMapping.resize(mGraphicsContext, newResolution);
}

void Renderer3D::trackPath(Core::TrackPathID id, const char *path) {
  mGraphicsContext.device().idle();

  ResourceTracker *trackers[] = {};

  // Add other file trackers after
  for (int i = 0; i < sizeof(trackers) / sizeof(trackers[0]); ++i) {
    trackers[i]->trackPath(mGraphicsContext, id, path);
  }
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mToneMapping;
}

Scene *Renderer3D::createScene() {
  Scene *ret = new Scene(mModelManager, mRenderMethods);
  ret->init(mGBuffer, mGraphicsContext);
  return ret;
}

void Renderer3D::bindScene(Scene *scene) {
  // This sucks
#if 0
  if (mBoundScene != scene) {
    mTerrainRenderer.forceFullUpdate();
  }
#endif

  mBoundScene = scene;
}

}
