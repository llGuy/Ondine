#include <thread>
#include "yona_time.hpp"

namespace Yona::Time {

TimeStamp getCurrentTime() {
    return std::chrono::high_resolution_clock::now();
}

float getTimeDifference(
    TimeStamp end,
    TimeStamp start) {
    std::chrono::duration<float> seconds = end - start;
    float delta = seconds.count();
    return (float)delta;
}

void sleepSeconds(float seconds) {
    std::this_thread::sleep_for(
      std::chrono::milliseconds((uint32_t)(seconds * 1000.0f)));
}

}
