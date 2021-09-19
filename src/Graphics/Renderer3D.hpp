#pragma once

#include "IO.hpp"
#include "Event.hpp"
#include "Utils.hpp"
#include "Scene.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "GBuffer.hpp"
#include "Delegate.hpp"
#include "Clipping.hpp"
#include "Pixelater.hpp"
#include "SkyRenderer.hpp"
#include "ToneMapping.hpp"
#include "RenderStage.hpp"
#include "ModelManager.hpp"
#include "RenderMethod.hpp"
#include "StarRenderer.hpp"
#include "BloomRenderer.hpp"
#include "WaterRenderer.hpp"
#include "VulkanContext.hpp"
#include "TerrainRenderer.hpp"
#include "AnimationManager.hpp"
#include "DeferredLighting.hpp"
#include "VulkanArenaAllocator.hpp"

namespace Ondine::View {

class EditorView;
class MapView;

}

namespace Ondine::Graphics {

class Renderer3D :
  public DelegateResize {
public:
  Renderer3D(
    VulkanContext &graphicsContext);

  void init();
  void shutdown();
  void tick(const Core::Tick &tick, Graphics::VulkanFrame &frame);
  void resize(Resolution newResolution) override;
  void trackPath(Core::TrackPathID id, const char *path);
  const RenderStage &mainRenderStage() const;

public:
  Scene *createScene();
  void bindScene(Scene *scene);

  inline PlanetProperties &planet() {
    return mPlanetProperties;
  }

public:
  Resolution pipelineViewport;

private:
  /* 
     Contains all the info about the 3D scene currently being rendered 
  */
  Scene *mBoundScene;

  /* 
     Various renderers for specific things 
  */
  StarRenderer mStarRenderer;
  SkyRenderer mSkyRenderer;
  WaterRenderer mWaterRenderer;
  TerrainRenderer mTerrainRenderer;
  PlanetRenderer mPlanetRenderer;

  /* 
     Rendering stages in the pipeline 
  */
  GBuffer mGBuffer;
  DeferredLighting mDeferredLighting;
  BloomRenderer mBloomRenderer;
  Pixelater mPixelater;
  ToneMapping mToneMapping;

  /* 
     Some uniform buffer / GPU storage 
  */
  Camera mCamera;
  Clipping mClipping;
  PlanetProperties mPlanetProperties;

  /* 
     A bunch of centralised locations for rendering objects. Allows for some
     optimisation - mostly memory related.
  */
  RenderMethodEntries mRenderMethods;
  RenderShaderEntries mShaderEntries;
  ModelManager mModelManager;
  AnimationManager mAnimationManager;


  VulkanContext &mGraphicsContext;


  // Only relevant in DEV builds
  friend class View::EditorView;
  friend class View::MapView;
};

}
