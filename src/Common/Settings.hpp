#pragma once

namespace Ondine {

struct Settings {
  float maxFramerate;
  float minFrametime;

  // Helper
  inline void setMaxFramerate(float fps) {
    maxFramerate = fps;
    minFrametime = 1.0f / maxFramerate;
  }
};

extern Settings gSettings;

}
