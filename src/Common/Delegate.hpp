#pragma once

#include "IO.hpp"
#include "Tick.hpp"
#include "Utils.hpp"
#include "FileSystem.hpp"

namespace Ondine {

namespace Graphics {

class Geometry;

}

class DelegateResize {
public:
  virtual void resize(Resolution newResolution) = 0;
};

class DelegateTrackInput {
public:
  virtual void trackInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker) = 0;
};

class DelegateTickable {
public:
  virtual void tick(const Core::Tick &tick) = 0;
};

class DelegateGeometryManager {
public:
  virtual Graphics::Geometry &getGeometry(const char *name) = 0;
};

}
