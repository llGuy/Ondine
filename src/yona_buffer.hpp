#pragma once

#include <stdint.h>
#include <stdlib.h>

namespace Yona {

/* Fixed size */
struct Buffer {
  uint8_t *data;
  size_t size;
};

}
