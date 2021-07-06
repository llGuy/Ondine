#include "Pixelater.hpp"
#include "FileSystem.hpp"
#include "Application.hpp"
#include "RendererDebug.hpp"

namespace Ondine::Graphics {

const char *const Pixelater::PIXELATER_FRAG_SPV = "res/spv/Pixelater.frag.spv";

void Pixelater::init(
  VulkanContext &graphicsContext,
  const VkExtent2D &initialExtent) {
  pixelationStrength = 3.0f;

  { // Set tracked resources
    addTrackedPath(PIXELATER_FRAG_SPV, &mPipeline);
  }

  { // Create render pass
    VulkanRenderPassConfig renderPassConfig(1, 1);

    renderPassConfig.addAttachment(
      LoadAndStoreOp::ClearThenStore, LoadAndStoreOp::DontCareThenDontCare,
      OutputUsage::FragmentShaderRead, AttachmentType::Color,
      VK_FORMAT_R8G8B8A8_UNORM);

    renderPassConfig.addSubpass(
      makeArray<uint32_t, AllocationType::Linear>(0U),
      makeArray<uint32_t, AllocationType::Linear>(),
      false);

    mRenderPass.init(graphicsContext.device(), renderPassConfig);
  }

  { // Create pipeline
    mPipeline.init(
      [] (VulkanPipeline &res, Pixelater &owner, VulkanContext &graphicsContext) {
        Core::File vshFile = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          "res/spv/TexturedQuad.vert.spv",
          Core::FileOpenType::Binary | Core::FileOpenType::In);
        Buffer vsh = vshFile.readBinary();

        Core::File fshFile = Core::gFileSystem->createFile(
          (Core::MountPoint)Core::ApplicationMountPoints::Application,
          "res/spv/Pixelater.frag.spv",
          Core::FileOpenType::Binary | Core::FileOpenType::In);
        Buffer fsh = fshFile.readBinary();

        VulkanPipelineConfig pipelineConfig(
          {owner.mRenderPass, 0},
          VulkanShader(
            graphicsContext.device(), vsh, VulkanShaderType::Vertex),
          VulkanShader(
            graphicsContext.device(), fsh, VulkanShaderType::Fragment));

        pipelineConfig.configurePipelineLayout(
          sizeof(Pixelater::PushConstant),
          VulkanPipelineDescriptorLayout{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});

        res.init(
          graphicsContext.device(),
          graphicsContext.descriptorLayouts(),
          pipelineConfig);
      },
      *this,
      graphicsContext);
  }

  { // Create target
    mExtent = {
      initialExtent.width,
      initialExtent.height,
    };

    initTargets(graphicsContext);
  }
}

void Pixelater::render(VulkanFrame &frame, const RenderStage &prev) {
  auto &commandBuffer = frame.primaryCommandBuffer;

  commandBuffer.dbgBeginRegion("PixelatorStage", DBG_PIXELATOR_COLOR);

  commandBuffer.beginRenderPass(mRenderPass, mFBO, {}, mExtent);

  commandBuffer.bindPipeline(mPipeline.res);
  commandBuffer.bindUniforms(prev.uniform());

  Pixelater::PushConstant pushConstant = {};
  pushConstant.pixelationStrength = pixelationStrength;
  pushConstant.width = (float)mExtent.width;
  pushConstant.height = (float)mExtent.height;

  commandBuffer.pushConstants(sizeof(pushConstant), &pushConstant);

  commandBuffer.setViewport();
  commandBuffer.setScissor();

  commandBuffer.draw(4, 1, 0, 0);

  commandBuffer.endRenderPass();

  commandBuffer.dbgEndRegion();
}

void Pixelater::resize(VulkanContext &vulkanContext, Resolution newResolution) {
  destroyTargets(vulkanContext);
  mExtent = {newResolution.width, newResolution.height};
  initTargets(vulkanContext);
}

const VulkanRenderPass &Pixelater::renderPass() const {
  return mRenderPass;
}

const VulkanFramebuffer &Pixelater::framebuffer() const {
  return mFBO;
}

const VulkanUniform &Pixelater::uniform() const {
  return mOutput;
}

VkExtent2D Pixelater::extent() const {
  return mExtent;
}

void Pixelater::initTargets(VulkanContext &graphicsContext) {
  mTexture.init(
    graphicsContext.device(), TextureType::T2D | TextureType::Attachment,
    TextureContents::Color, VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_NEAREST,
    {mExtent.width, mExtent.height, 1}, 1, 1);

  mOutput.init(
    graphicsContext.device(),
    graphicsContext.descriptorPool(),
    graphicsContext.descriptorLayouts(),
    makeArray<VulkanTexture, AllocationType::Linear>(mTexture));

  VulkanFramebufferConfig fboConfig(1, mRenderPass);
  fboConfig.addAttachment(mTexture);
  mFBO.init(graphicsContext.device(), fboConfig);
}

void Pixelater::destroyTargets(VulkanContext &graphicsContext) {
  mOutput.destroy(
    graphicsContext.device(), graphicsContext.descriptorPool());
  mFBO.destroy(graphicsContext.device());
  mTexture.destroy(graphicsContext.device());
}

}
