#pragma once

namespace Yona {

class Application {
public:
  Application();
  virtual ~Application();

  void run();

private:
  virtual void start() = 0;
  virtual void tick() = 0;

private:
  bool mIsRunning;
};

}
