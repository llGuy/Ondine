#include "RenderMethod.hpp"

namespace Ondine::Graphics {

RenderMethod::RenderMethod(
  const ModelManager &models,
  const RenderShaderEntries &entries)
  : mShaderEntries(entries),
    mModelManager(models) {
  
}

void RenderMethod::init(
  const std::string &shaderName,
  StaticModelHandle handleModel,
  RenderBindResourceProc proc) {
  mRenderShader = mShaderEntries.getHandle(shaderName);
  mModel = handleModel;
  mBindResourceProc = proc;
}

void RenderMethod::prepare(const VulkanFrame &frame) {
  auto &pipeline = mShaderEntries.getEntry(mRenderShader);

  const auto &model = mModelManager.getStaticModel(mModel);
  model.bindIndexBuffer(frame.primaryCommandBuffer);
  model.bindVertexBuffers(frame.primaryCommandBuffer);
}

void RenderMethod::bindResources(
  const VulkanFrame &frame,
  const RenderResources &resources) {
  mBindResourceProc(frame.primaryCommandBuffer, resources);
}

}
