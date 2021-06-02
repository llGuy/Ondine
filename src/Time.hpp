#pragma once

#include <chrono>

namespace Ondine::Time {

using TimeStamp = std::chrono::high_resolution_clock::time_point;

TimeStamp getCurrentTime();
float getTimeDifference(TimeStamp end, TimeStamp start);
void sleepSeconds(float seconds);

}
