#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include "Log.hpp"

namespace Ondine::Core {

using JobID = int;
using JobProc = int (*)(void *parameters);

struct Job {
  JobID id;
  JobProc proc;
  void *parameters;
  int workerID;
  bool isRunning;
};

class Worker {
public:
  void init(int id);

  bool startIfIdle(Job job);
  bool isFinished();
  int getReturnCode();
  JobID getJobID();

private:
  void runtime();

private:
  int mID;
  Job mJob;
  int mReturnCode;
  bool mJobRequested;

  std::mutex mMutex;
  std::thread mThread;
  // Ready will stop waiting when job started is true
  std::condition_variable mReady;
};

}
