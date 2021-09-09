#include "RenderMethod.hpp"

namespace Ondine::Graphics {

RenderMethod::RenderMethod(
  const ModelManager &models,
  const RenderShaderEntries &entries)
  : mShaderEntries(&entries),
    mModelManager(&models) {
  
}

void RenderMethod::init(
  const std::string &shaderName,
  ModelHandle handleModel,
  RenderBindResourceProc bindResProc,
  RenderPushConstantProc pushConstantProc) {
  mRenderShader = mShaderEntries->getHandle(shaderName);
  mModel = handleModel;
  mBindResourceProc = bindResProc;
  mPushConstantProc = pushConstantProc;
}

void RenderMethod::bindShader(const VulkanFrame &frame) {
  auto &shader = mShaderEntries->getEntry(mRenderShader);
  frame.primaryCommandBuffer.bindPipeline(shader);
}

void RenderMethod::bindBuffers(const VulkanFrame &frame) {
  auto &pipeline = mShaderEntries->getEntry(mRenderShader);

  const auto &model = mModelManager->getModel(mModel);
  model.bindIndexBuffer(frame.primaryCommandBuffer);
  model.bindVertexBuffers(frame.primaryCommandBuffer);
}

void RenderMethod::bindResources(
  const VulkanFrame &frame,
  const RenderResources &resources) {
  mBindResourceProc(frame.primaryCommandBuffer, resources);
}

void RenderMethod::pushConstant(
  const VulkanFrame &frame,
  const struct SceneObject &object) {
  mPushConstantProc(frame.primaryCommandBuffer, object);
}

void RenderMethod::submit(const VulkanFrame &frame) {
  const auto &model = mModelManager->getModel(mModel);
  model.submitForRenderIndexed(frame.primaryCommandBuffer);
}

}
