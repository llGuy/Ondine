#pragma once

#include "Application.hpp"

namespace Ondine::Core {

class Client : public Application {
public:
  Client(int argc, char **argv);
  ~Client() override;

private:
  void start() override;
  void tick() override;
};

}
