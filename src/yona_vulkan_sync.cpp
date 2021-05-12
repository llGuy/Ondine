#include "yona_log.hpp"
#include "yona_utils.hpp"
#include "yona_vulkan_sync.hpp"

namespace Yona {

VkAccessFlags findAccessFlagsForStage(VkPipelineStageFlags stage) {
  switch (stage) {
  case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT;

  case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:
  case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:
    return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

  case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:
    return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

  case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:
    return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

  case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:
  case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
  case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
    return VK_ACCESS_UNIFORM_READ_BIT;

  case VK_PIPELINE_STAGE_TRANSFER_BIT:
    return VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;

  default: {
    LOG_ERROR("Didn't handle stage for finding access flags!\n");
    PANIC_AND_EXIT();
  } return 0;
  }
}

}
