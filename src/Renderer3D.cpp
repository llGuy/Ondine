#include "Buffer.hpp"
#include "Renderer3D.hpp"
#include "VulkanRenderPass.hpp"

namespace Ondine {

Renderer3D::Renderer3D(
  VulkanContext &graphicsContext,
  const InputTracker &inputTracker)
  : mGraphicsContext(graphicsContext),
    mInputTracker(inputTracker) {
  
}

void Renderer3D::init() {
  mCamera.init(mGraphicsContext);
  mGBuffer.init(mGraphicsContext);
  mSkyRenderer.init(mGraphicsContext, mGBuffer);

  // Idle with all precomputation stuff
  mGraphicsContext.device().graphicsQueue().idle();

  auto properties = mGraphicsContext.getProperties();
  mPipelineViewport = {
    properties.swapchainExtent.width, properties.swapchainExtent.height
  };

  /* Temporary - the world needs to define this */
  mPlanetProperties.solarIrradiance = glm::vec3(1.474f, 1.8504f, 1.91198f);
  // Angular radius of the Sun (radians)
  mPlanetProperties.solarAngularRadius = 0.004675f;
  mPlanetProperties.bottomRadius = 6360.0f;
  mPlanetProperties.topRadius = 6420.0f;

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
  mPlanetProperties.groundAlbedo = glm::vec3(0.100000f, 0.100000f, 0.100000f);
  mPlanetProperties.muSunMin = -0.207912f;

  mPlanetRenderer.init(mGraphicsContext, mGBuffer, &mPlanetProperties);
}

void Renderer3D::tick(const Tick &tick, VulkanFrame &frame) {
  mCameraProperties.aspectRatio =
    (float)mPipelineViewport.width / (float)mPipelineViewport.height;
     
  mGBuffer.beginRender(frame);
  {
    // Renders the demo
    mSkyRenderer.tick(tick, frame, mCameraProperties);
  }
  mGBuffer.endRender(frame);
}

void Renderer3D::resize(Resolution newResolution) {
  mGraphicsContext.device().idle();

  mPipelineViewport = {
    newResolution.width, newResolution.height
  };

  mGBuffer.resize(mGraphicsContext, newResolution);
}

const RenderStage &Renderer3D::mainRenderStage() const {
  return mGBuffer;
}

}
