#include "Worker.hpp"

namespace Ondine::Core {

void Worker::init(int id) {
  mID = id;
  mJobRequested = false;
  mThread = std::thread([this] () {runtime();});
}

void Worker::runtime() {
  LOG_INFOV("Worker %d has started a running\n", mID);

  for (;;) {
    std::unique_lock<std::mutex> lock (mMutex);
    mReady.wait(lock, [this] {return mJobRequested;});
    lock.unlock();

    LOG_INFOV("Worker %d has started a job\n", mID);

    int returnCode = mJob.proc(mJob.parameters);

    lock.lock();
    mJobRequested = false;
    mReturnCode = returnCode;
    lock.unlock();

    LOG_INFOV("Worker %d has finished a job\n", mID);
  }
}

bool Worker::startIfIdle(Job job) {
  std::unique_lock<std::mutex> lock (mMutex);

  if (mJobRequested) {
    // If a job was requested, the thread is working
    return false;
  }
  else {
    // Otherwise, no job was requested, the thread is free
    mJob = job;
    mJobRequested = true;
    mReady.notify_one();

    return true;
  }
}

bool Worker::isFinished() {
  std::unique_lock<std::mutex> lock (mMutex);
  return !mJobRequested;
}

int Worker::getReturnCode() {
  std::unique_lock<std::mutex> lock (mMutex);
  return !mReturnCode;
}

JobID Worker::getJobID() {
  return mJob.id;
}

}
