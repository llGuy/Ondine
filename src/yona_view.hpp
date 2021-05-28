#pragma once

#include "yona_vulkan_uniform.hpp"

namespace Yona {

class EventQueue;
class Tick;
class VulkanFrame;

class View {
public:
  View() = default;
  virtual ~View() {};

  virtual void processEvents(EventQueue &queue, const Tick &tick) = 0;

  virtual void render(
    const VulkanUniform &previousOutput,
    const VulkanFrame &frame, const Tick &tick) = 0;

  virtual const VulkanUniform &getOutput() const = 0;
};

}
