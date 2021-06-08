#include "WaterRenderer.hpp"

namespace Ondine::Graphics {

void WaterRenderer::init() {
  
}

void WaterRenderer::tick(VulkanFrame &frame) {
  mGBuffer.beginRender(frame);
  { // Render 3D scene
    
  }
  mGBuffer.endRender(frame);
}

}
