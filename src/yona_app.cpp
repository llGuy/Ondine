#include "yona_app.hpp"

namespace Yona {

Application::Application() {
  /* Initialise graphics context, etc... */
}

Application::~Application() {
  /* Destroy graphics context, etc... */
}

void Application::run() {
  /* User-defined function which will be overriden */
  start();

  while (mIsRunning) {
    
  }
}

}
