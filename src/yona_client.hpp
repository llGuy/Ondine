#pragma once

#include "yona_app.hpp"

namespace Yona {

class Client : public Application {
public:
  Client();
  ~Client() override;

private:
  void start() override;
  void tick() override;
};

}
