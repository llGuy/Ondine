#include "ThreadPool.hpp"

namespace Ondine::Core {

ThreadPool *gThreadPool;

void ThreadPool::init() {
  // Pre-allocate the threads that the application will be using
  mCoreCount = std::thread::hardware_concurrency();
  mWorkers.init(mCoreCount);
  mWorkers.size = mCoreCount;
  mJobs.init(30);

  for (int i = 0; i < mWorkers.size; ++i) {
    mWorkers[i].init(i);
  }
}

void ThreadPool::tick() {
  for (int i = 0; i < mWorkers.size; ++i) {
    if (mWorkers[i].isFinished()) {
      mJobs[mWorkers[i].getJobID()].isRunning = false;
    }
  }
}

JobID ThreadPool::createJob(JobProc proc) {
  auto key = mJobs.add({0, proc, nullptr, -1});
  mJobs[key].id = key;
  return key;
}

void ThreadPool::destroyJob(JobID jobID) {
  mJobs.remove(jobID);
}

void ThreadPool::startJob(JobID job, void *parameters) {
  for (int i = 0; i < mWorkers.size; ++i) {
    // Yes this is stupid but we'll fix later
    if (mWorkers[i].isFinished()) {
      mJobs[job].parameters = parameters;

      mWorkers[i].startIfIdle(mJobs[job]);

      mJobs[job].workerID = i;
      mJobs[job].isRunning = true;

      break;
    }
  }
}

bool ThreadPool::isJobFinished(JobID jobID) {
  return !mJobs[jobID].isRunning ||
    mWorkers[mJobs[jobID].workerID].isFinished();
}

int ThreadPool::getJobStatus(JobID jobID) {
  return mWorkers[mJobs[jobID].workerID].getReturnCode();
}
  
}
