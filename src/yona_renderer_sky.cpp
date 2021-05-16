#include "yona_app.hpp"
#include "yona_filesystem.hpp"
#include "yona_renderer_sky.hpp"
#include "yona_vulkan_context.hpp"
#include "yona_vulkan_texture.hpp"

namespace Yona {

void RendererSky::init(VulkanContext &graphicsContext) {
  
}

void RendererSky::preparePrecompute(VulkanContext &graphicsContext) {
  File precomputeVshFile = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_precompute.vert.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer quadVsh = precomputeVshFile.readBinary();

  prepareTransmittancePrecompute(quadVsh, graphicsContext);
}

void RendererSky::prepareTransmittancePrecompute(
  const Buffer &precomputeVsh,
  VulkanContext &graphicsContext) {
  VulkanRenderPassConfig renderPassConfig(1, 1);

  renderPassConfig.addAttachment(
    LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
    OutputUsage::FragmentShaderRead, AttachmentType::Color,
    VK_FORMAT_R32G32B32A32_SFLOAT);

  renderPassConfig.addSubpass(
    makeArray<uint32_t, AllocationType::Linear>(0U),
    makeArray<uint32_t, AllocationType::Linear>(),
    false);

  mPrecomputeTransmittanceRenderPass.init(
    graphicsContext.device(), renderPassConfig);

  File precomputeTransmittance = gFileSystem->createFile(
    (MountPoint)ApplicationMountPoints::Application,
    "res/spv/sky_transmittance.frag.spv",
    FileOpenType::Binary | FileOpenType::In);

  Buffer transmittance = precomputeTransmittance.readBinary();

  VulkanPipelineConfig pipelineConfig(
    {mPrecomputeTransmittanceRenderPass, 0},
    VulkanShader(
      graphicsContext.device(), precomputeVsh, VulkanShaderType::Vertex),
    VulkanShader(
      graphicsContext.device(), transmittance, VulkanShaderType::Fragment));

  pipelineConfig.configurePipelineLayout(
    0, VulkanPipelineDescriptorLayout{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

  mPrecomputeTransmittancePipeline.init(
    graphicsContext.device(),
    graphicsContext.descriptorLayouts(),
    pipelineConfig);

  mPrecomputedTransmittance.init(
    graphicsContext.device(), TextureType::T2D, TextureContents::Color,
    VK_FORMAT_R32G32B32A32_SFLOAT, VK_FILTER_LINEAR, {}, 1, 1);

  VulkanFramebufferConfig fboConfig(1, mPrecomputeTransmittanceRenderPass);
  fboConfig.addAttachment(mPrecomputedTransmittance);

  mPrecomputedTransmittanceFBO.init(graphicsContext.device(), fboConfig);
}

}
