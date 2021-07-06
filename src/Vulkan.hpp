#pragma once

#include "Log.hpp"
#include <vulkan/vulkan.h>

/* Some utility stuff for vulkan */
#define VK_CHECK(call) \
  if (call != VK_SUCCESS) { LOG_ERRORV("%s failed\n", #call); }

extern PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag;
extern PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName;
extern PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin;
extern PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd;
extern PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert;
