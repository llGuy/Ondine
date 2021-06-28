#pragma once

#include "IO.hpp"
#include "Tick.hpp"
#include "Utils.hpp"
#include "FileSystem.hpp"

namespace Ondine {

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

/* For tracking whether file has changed (reload resources for instance...) */
class DelegateTrackFile {
public:
  virtual void trackFile(Core::TrackFileID id, Core::File &file) = 0;
};

}
