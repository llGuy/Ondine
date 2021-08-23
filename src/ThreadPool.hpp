#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include "Log.hpp"
#include "Buffer.hpp"
#include "Worker.hpp"
#include "NumericMap.hpp"

namespace Ondine::Core {

using JobID = int;

class ThreadPool {
public:
  void init();
  void tick();

  JobID createJob(JobProc proc);
  void destroyJob(JobID jobID);

  // TODO: Change this to queueJob when parallelism becomes more prevalent
  void startJob(JobID jobID, void *parameters);
  bool isJobFinished(JobID jobID);
  int getJobStatus(JobID jobID);

private:
  int mCoreCount;
  Array<Worker> mWorkers;
  NumericMap<Job> mJobs;
};

extern ThreadPool *gThreadPool;

}
