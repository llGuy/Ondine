#pragma once

#include "Tick.hpp"
#include "Camera.hpp"
#include "VulkanFrame.hpp"
#include "RenderStage.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanUniform.hpp"
#include "PlanetRenderer.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanRenderPass.hpp"
#include "VulkanFramebuffer.hpp"

namespace Ondine::View {

class EditorView;

}

namespace Ondine::Graphics {

class VulkanContext;

class SkyRenderer {
public:
  void init(
    VulkanContext &graphicsContext,
    const RenderStage &renderStage);

  void shutdown(VulkanContext &graphicsContext);

  void tick(
    const Core::Tick &tick,
    VulkanFrame &frame,
    const CameraProperties &camera);

  const VulkanUniform &uniform() const;

private:
  void initSkyProperties(VulkanContext &graphicsContext);
  void initFinalTextures(VulkanContext &graphicsContext);
  void initTemporaryPrecomputeTextures(VulkanContext &graphicsContext);

  void preparePrecompute(VulkanContext &graphicsContext);

  void prepareTransmittancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void prepareSingleScatteringPrecompute(
    const Buffer &precomputeVsh,
    const Buffer &precomputeGsh,
    VulkanContext &graphicsContext);

  void prepareDirectIrradiancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void prepareIndirectIrradiancePrecompute(
    const Buffer &precomputeVsh,
    VulkanContext &graphicsContext);

  void prepareScatteringDensityPrecompute(
    const Buffer &precomputeVsh,
    const Buffer &precomputeGsh,
    VulkanContext &graphicsContext);

  void prepareMultipleScatteringPrecompute(
    const Buffer &precomputeVsh,
    const Buffer &precomputeGsh,
    VulkanContext &graphicsContext);

  void precompute(VulkanContext &graphicsContext);
  void precomputeTransmittance(VulkanCommandBuffer &commandBuffer);
  void precomputeSingleScattering(VulkanCommandBuffer &commandBuffer);
  void precomputeDirectIrradiance(VulkanCommandBuffer &commandBuffer);
  void precomputeIndirectIrradiance(
    VulkanCommandBuffer &commandBuffer, int scatteringOrder);
  void precomputeScatteringDensity(
    VulkanCommandBuffer &commandBuffer,
    uint32_t splitIndex, int scatteringOrder,
    uint32_t startLayer, uint32_t endLayer);
  void precomputeMultipleScattering(
    VulkanCommandBuffer &commandBuffer,
    uint32_t splitIndex, int scatteringOrder,
    uint32_t startLayer, uint32_t endLayer);

private:
  // Some utility functions
  void make3DTextureAndUniform(
    const VkExtent3D extent,
    VulkanTexture &texture, VulkanUniform &uniform,
    VulkanContext &graphicsContext,
    bool isTemporary);

  void make2DTextureAndUniform(
    const VkExtent3D extent,
    VulkanTexture &texture, VulkanUniform &uniform,
    VulkanContext &graphicsContext);

  void initDemoPipeline(
    VulkanContext &graphicsContext,
    const RenderStage &renderStage);

  bool isPrecomputationNeeded() const;

  void loadFromCache(VulkanContext &graphicsContext);
  void saveToCache(VulkanContext &graphicsContext);

private:
  static constexpr size_t NUM_SCATTERING_ORDERS = 4;
  static constexpr size_t TRANSMITTANCE_WIDTH = 256;
  static constexpr size_t TRANSMITTANCE_HEIGHT = 64;
  static constexpr size_t SCATTERING_TEXTURE_R_SIZE = 32;
  static constexpr size_t SCATTERING_TEXTURE_MU_SIZE = 128;
  static constexpr size_t SCATTERING_TEXTURE_MU_S_SIZE = 32;
  static constexpr size_t SCATTERING_TEXTURE_NU_SIZE = 8;
  static constexpr size_t SCATTERING_TEXTURE_WIDTH =
    SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
  static constexpr size_t SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
  static constexpr size_t SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;
  static constexpr size_t IRRADIANCE_TEXTURE_WIDTH = 64;
  static constexpr size_t IRRADIANCE_TEXTURE_HEIGHT = 16;
  static constexpr VkFormat PRECOMPUTED_TEXTURE_FORMAT32 =
    VK_FORMAT_R32G32B32A32_SFLOAT;
  static constexpr VkFormat PRECOMPUTED_TEXTURE_FORMAT16 =
    VK_FORMAT_R16G16B16A16_SFLOAT;

  PlanetProperties mSkyProperties;
  VulkanBuffer mSkyPropertiesBuffer;
  VulkanUniform mSkyPropertiesUniform;

  VulkanPipeline mPrecomputeTransmittancePipeline;
  VulkanRenderPass mPrecomputeTransmittanceRenderPass;
  VulkanFramebuffer mPrecomputeTransmittanceFBO;
  VulkanTexture mPrecomputedTransmittance;
  VulkanUniform mPrecomputedTransmittanceUniform;

  struct PrecomputePushConstant {
    int layer;
    int scatteringOrder;
  };

  struct DemoPushConstant {
    float modelFromView[16];
    float viewFromClip[16];

    alignas(16) glm::vec3 camera;
    alignas(16) glm::vec3 whitePoint;
    alignas(16) glm::vec3 earthCenter;
    alignas(16) glm::vec3 sunDirection;
    alignas(16) glm::vec2 sunSize;
    float exposure;
  };

  VulkanPipeline mPrecomputeSingleScatteringPipeline;
  VulkanRenderPass mPrecomputeSingleScatteringRenderPass;
  VulkanFramebuffer mPrecomputeSingleScatteringFBO;

  VulkanTexture mPrecomputedScattering;
  VulkanUniform mPrecomputedScatteringUniform;

  VulkanPipeline mPrecomputeDirectIrradiancePipeline;
  VulkanRenderPass mPrecomputeDirectIrradianceRenderPass;
  VulkanFramebuffer mPrecomputeDirectIrradianceFBO;

  VulkanPipeline mPrecomputeIndirectIrradiancePipeline;

  VulkanTexture mPrecomputedIrradiance;
  VulkanUniform mPrecomputedIrradianceUniform;

  struct SplitPrecomputation {
    VulkanPipeline pipeline[2];
    VulkanRenderPass renderPass[2];
    VulkanFramebuffer fbo[2];

    // Function to prepare just one split
    template <typename Proc>
    void prepare(Proc prepareProc) {
      prepareProc(
        LoadAndStoreOp::ClearThenStore,
        OutputUsage::None,
        pipeline[0], renderPass[0], fbo[0]);

      prepareProc(
        LoadAndStoreOp::LoadThenStore,
        OutputUsage::FragmentShaderRead,
        pipeline[1], renderPass[1], fbo[1]);
    }
  };

  SplitPrecomputation mPrecomputeScatteringDensity;
  SplitPrecomputation mPrecomputeMultipleScattering;

  /* Temporary textures */
  union {
    VulkanTexture mDeltaMultipleScatteringTexture;
    VulkanTexture mDeltaRayleighScatteringTexture;
  };

  union {
    VulkanUniform mDeltaMultipleScatteringUniform;
    VulkanUniform mDeltaRayleighScatteringUniform;
  };

  VulkanTexture mDeltaMieScatteringTexture;
  VulkanUniform mDeltaMieScatteringUniform;

  VulkanTexture mDeltaIrradianceTexture;
  VulkanUniform mDeltaIrradianceUniform;

  VulkanTexture mDeltaScatteringDensityTexture;
  VulkanUniform mDeltaScatteringDensityUniform;

  /* Uniform that gets passed to rendering operations which need the sky */
  VulkanUniform mRenderingUniform;

  /* For now, contain the demo shader */
  VulkanPipeline mDemo;

  /* Should we save precomputation textures to cache? */
  bool mCache;

  float mViewDistanceMeters = 9000.000000;
  float mViewZenithAngleRadians = 1.470000;
  float mViewAzimuthAngleRadians = 0.000000;
  float mSunZenithAngleRadians = 1.300000;
  float mSunAzimuthAngleRadians = 3.000000;
  float mExposure = 10.000000;

  friend class View::EditorView;
};

}
