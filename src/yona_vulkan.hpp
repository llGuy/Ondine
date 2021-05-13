#pragma once

#include "yona_log.hpp"

/* Some utility stuff for vulkan */
#define VK_CHECK(call) \
  if (call != VK_SUCCESS) { LOG_ERRORV("%s failed\n", #call); }
