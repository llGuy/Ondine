#pragma once

#include <vulkan/vulkan.h>

namespace Yona {

VkAccessFlags findAccessFlagsForStage(VkPipelineStageFlags stage);

}
