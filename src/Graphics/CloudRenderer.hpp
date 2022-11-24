#pragma once

#include "VulkanUniform.hpp"

namespace Ondine::Graphics {

class RenderStage;
class VulkanContext;

class CloudRenderer {
public:
  void init(
    VulkanContext &graphicsContext,
    const RenderStage &renderStage);

  void shutdown(VulkanContext &graphicsContext);

  const VulkanUniform &uniform() const;

private:

};

}
