#include <iostream>
#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "FileSystem.hpp"
#include "RendererCache.hpp"
#include "RendererDebug.hpp"
#include "AssimpImporter.hpp"
#include "VulkanRenderPass.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/random.hpp>
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
  mAnimationManager.init();
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
    mPlanetProperties.bottomRadius = 6360.0f / 2.0f;
    mPlanetProperties.topRadius = 6420.0f / 2.0f;

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
  mToneMapping.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height},
    {glm::vec4(2.0f, 2.0f, 2.0f, 1.0f), 20.0f});
  mClipping.init(
    mGraphicsContext,
    -1.0f,
    mPlanetProperties.bottomRadius + 1.0f);

  { // Testing animation system
    const aiScene *model = importScene("res/model/ondine/Ondine.fbx");

    ModelConfig ondineConfig = mModelManager.loadModelConfig(
      model, mGraphicsContext);
    mAnimationManager.loadSkeleton(model, ondineConfig, mGraphicsContext);
    // auto ondineHandle = 
  }

  { // Prepare scene resources
    /* Create model */
    ModelConfig sphereModelConfig = mModelManager.loadModelConfig(
      "res/model/UVSphere.fbx", mGraphicsContext);
    auto sphereModelHandle = mModelManager.createModel(
      sphereModelConfig, mGraphicsContext);

    ModelConfig taurusModelConfig = mModelManager.loadModelConfig(
      "res/model/Taurus.fbx", mGraphicsContext);
    auto taurusModelHandle = mModelManager.createModel(
      taurusModelConfig, mGraphicsContext);
    
    { // Create base model shader
      VulkanPipelineConfig pipelineConfig(
        {mGBuffer.renderPass(), 0},
        VulkanShader{mGraphicsContext.device(), "res/spv/BaseModel.vert.spv"},
        VulkanShader{mGraphicsContext.device(), "res/spv/BaseModel.frag.spv"});

      pipelineConfig.enableDepthTesting();
      pipelineConfig.configurePipelineLayout(
        sizeof(SceneObject::pushConstant),
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

      sphereModelConfig.configureVertexInput(pipelineConfig);
      pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

      auto &baseModelShader = mShaderEntries.emplace("BaseModelShader");
      
      baseModelShader.init(
        mGraphicsContext.device(),
        mGraphicsContext.descriptorLayouts(),
        pipelineConfig);
    }

    { // Create glowing shader
      VulkanPipelineConfig pipelineConfig(
        {mGBuffer.renderPass(), 0},
        VulkanShader{mGraphicsContext.device(), "res/spv/BaseModel.vert.spv"},
        VulkanShader{mGraphicsContext.device(), "res/spv/Glowing.frag.spv"});

      pipelineConfig.enableDepthTesting();
      pipelineConfig.configurePipelineLayout(
        sizeof(SceneObject::pushConstant),
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

      sphereModelConfig.configureVertexInput(pipelineConfig);
      pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

      auto &baseModelShader = mShaderEntries.emplace("GlowingModelShader");
      
      baseModelShader.init(
        mGraphicsContext.device(),
        mGraphicsContext.descriptorLayouts(),
        pipelineConfig);
    }

    { // Points
      VulkanPipelineConfig pipelineConfig(
        {mGBuffer.renderPass(), 0},
        VulkanShader{mGraphicsContext.device(), "res/spv/Point.vert.spv"},
        VulkanShader{mGraphicsContext.device(), "res/spv/Point.frag.spv"});

      pipelineConfig.configurePipelineLayout(
        sizeof(SceneObject::pushConstant));

      pipelineConfig.setTopology(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

      mPointPipeline.init(
        mGraphicsContext.device(),
        mGraphicsContext.descriptorLayouts(),
        pipelineConfig);

      mParticles.init(PARTICLE_COUNT);
      mCircles.init(MAX_CIRCLE_COUNT);

      float firstCircleRadius = 0.5f;
      mCircles.add(Circle{glm::vec2(0.0f), firstCircleRadius});

      for (int i = 0; i < mParticles.capacity; ++i) {
        mParticles[i].random = glm::linearRand(0.0f, 1.0f);

        mParticles[i].dstPosition = firstCircleRadius * glm::vec2(
          glm::cos(glm::radians(mParticles[i].random * 360.0f)),
          glm::sin(glm::radians(mParticles[i].random * 360.0f)));
        mParticles[i].currentPosition = mParticles[i].dstPosition +
          glm::linearRand(glm::vec2(-1.0f), glm::vec2(1.0f)) * 0.01f;

        mParticles[i].color = glm::vec4(1.8f, 0.9f, 2.85f, 1.0f) * 5.0f;
        mParticles[i].velocity = glm::vec2(0.0f);
      }
    }

    /* Create render method */
    RenderMethod baseModelMethod(mModelManager, mShaderEntries);
    baseModelMethod.init(
      "BaseModelShader", sphereModelHandle,
      [](const VulkanCommandBuffer &cmdbuf, const RenderResources &res) {
        cmdbuf.bindUniforms(
          res.camera.uniform(), res.planet.uniform(), res.clipping.uniform);
      },
      [](const VulkanCommandBuffer &cmdbuf, const SceneObject &obj) {
        cmdbuf.pushConstants(sizeof(obj.pushConstant), &obj.pushConstant);
      });

    mRenderMethods.insert("SphereModelRenderMethod", baseModelMethod);

    RenderMethod taurusModelMethod(mModelManager, mShaderEntries);
    taurusModelMethod.init(
      "BaseModelShader", taurusModelHandle,
      [](const VulkanCommandBuffer &cmdbuf, const RenderResources &res) {
        cmdbuf.bindUniforms(
          res.camera.uniform(), res.planet.uniform(), res.clipping.uniform);
      },
      [](const VulkanCommandBuffer &cmdbuf, const SceneObject &obj) {
        cmdbuf.pushConstants(sizeof(obj.pushConstant), &obj.pushConstant);
      });

    mRenderMethods.insert("TaurusModelRenderMethod", taurusModelMethod);

    RenderMethod glowingModelMethod(mModelManager, mShaderEntries);
    glowingModelMethod.init(
      "GlowingModelShader", taurusModelHandle,
      [](const VulkanCommandBuffer &cmdbuf, const RenderResources &res) {
        cmdbuf.bindUniforms(
          res.camera.uniform(), res.planet.uniform(), res.clipping.uniform);
      },
      [](const VulkanCommandBuffer &cmdbuf, const SceneObject &obj) {
        cmdbuf.pushConstants(sizeof(obj.pushConstant), &obj.pushConstant);
      });

    mRenderMethods.insert("GlowingTaurusRenderMethod", glowingModelMethod);
  }

  mWaterRenderer.init(mGraphicsContext, mPlanetProperties);

  mPixelater.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height});

  mBloomRenderer.init(
    mGraphicsContext,
    {pipelineViewport.width, pipelineViewport.height});
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
  mTerrainRenderer.sync(
    mBoundScene->terrain,
    mBoundScene->camera,
    frame.primaryCommandBuffer);

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

    auto &commandBuffer = frame.primaryCommandBuffer;

    commandBuffer.bindPipeline(mPointPipeline);

    commandBuffer.setViewport();
    commandBuffer.setScissor();

    struct PushConstant {
      glm::vec4 position;
      glm::vec4 color;
      float fade;
      float starSize;
    };

    mFade = 1.0;

    float radius = 0.5f / (float)mCircles.size();

    int particleIdx = 0;
    int particlesPerCircle = 1 + mParticles.capacity / mCircles.size();

    for (auto &circle : mCircles) {
      int startIdx = particleIdx;
      int endIdx = particleIdx + particlesPerCircle;

      for (; particleIdx < endIdx && particleIdx < mParticles.capacity; ++particleIdx) {
        auto &particle = mParticles[particleIdx];

        float circleProgress = particle.random;

        particle.dstPosition = circle.center + radius * glm::vec2(
          glm::cos(glm::radians(circleProgress * 360.0f)),
          glm::sin(glm::radians(circleProgress * 360.0f)));

        glm::vec2 diff = (particle.dstPosition - particle.currentPosition);
        float magSq = glm::dot(diff, diff);

        glm::vec2 acc = diff * magSq;

        particle.velocity += acc * tick.dt;
        particle.currentPosition += particle.velocity * tick.dt;

        PushConstant pushConstant = {
          glm::vec4(
            particle.currentPosition.x,
            particle.currentPosition.y,
            0.0f, 1.0f),

          particle.color,
          mFade,
          3.0f
        };

        commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

        commandBuffer.draw(1, 1, 0, 0);
      }
    }
  }

  mGBuffer.endRender(frame);

  mDeferredLighting.render(
    frame, mGBuffer, mCamera, mPlanetRenderer, mWaterRenderer, mSkyRenderer);

  mPixelater.render(frame, mDeferredLighting);

  mBloomRenderer.render(frame, mDeferredLighting);

  mToneMapping.render(frame, mBloomRenderer, mPixelater);
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

  mBloomRenderer.resize(mGraphicsContext, newResolution);

  mToneMapping.resize(mGraphicsContext, newResolution);
}

void Renderer3D::trackPath(Core::TrackPathID id, const char *path) {
  mGraphicsContext.device().idle();

  ResourceTracker *trackers[] = {
    &mPixelater,
    &mDeferredLighting,
    &mToneMapping,
    &mBloomRenderer
  };

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
  if (mBoundScene != scene) {
    mTerrainRenderer.forceFullUpdate();
  }

  mBoundScene = scene;
}

}
