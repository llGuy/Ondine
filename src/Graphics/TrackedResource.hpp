#pragma once

#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Graphics {

class VulkanContext;

template <typename T, typename Owner>
using TrackedResourceInitProc = void (*)(
  T &res, Owner &owner,
  VulkanContext &graphicsContext);

class TrackedResourceInterface {
public:
  virtual void init(VulkanContext &graphicsContext) = 0;

  // May need to make this a bit more customisable
  virtual void destroy(VulkanContext &graphicsContext) = 0;
};

template <typename T, typename Owner>
class TrackedResource : public TrackedResourceInterface {
public:
  void init(
    TrackedResourceInitProc<T, Owner> initProc,
    Owner &owner,
    VulkanContext &graphicsContext) {
    mProc = initProc;
    mProc(res, owner, graphicsContext);
    mOwner = &owner;
  }
  
  void init(VulkanContext &graphicsContext) {
    mProc(res, *mOwner, graphicsContext);
  }

  // May need to make this a bit more customisable
  void destroy(VulkanContext &graphicsContext) override {
    res.destroy(graphicsContext.device());
  }

  T res;
private:

  TrackedResourceInitProc<T, Owner> mProc;
  Owner *mOwner;
};

class ResourceTracker {
public:
  void trackPath(
    VulkanContext &graphicsContext,
    Core::TrackPathID id,
    const char *path);

  void addTrackedPath(const char *path, TrackedResourceInterface *res);

private:
  std::unordered_map<Core::TrackPathID, TrackedResourceInterface *> mRefs;
};

}

