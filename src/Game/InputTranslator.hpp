#pragma once

#include "IO.hpp"
#include "GameInput.hpp"

namespace Ondine::Game {

// Also tracks input settings for the user
class InputTranslator {
public:
  UserInput translate(const Core::InputTracker &inputTracker);

private:
  // TODO: Input settings
};

}
