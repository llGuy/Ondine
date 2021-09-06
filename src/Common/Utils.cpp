#include "Utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Ondine {

ImagePixels getImagePixelsFromBuffer(const Buffer &data) {
  ImagePixels ret = {};
  ret.data = stbi_load_from_memory(
    data.data,
    data.size,
    &ret.width,
    &ret.height,
    &ret.channelCount,
    STBI_rgb_alpha);

  return ret;
}

}
