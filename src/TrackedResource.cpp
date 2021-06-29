#include "Application.hpp"
#include "TrackedResource.hpp"

namespace Ondine::Graphics {

void ResourceTracker::trackPath(
  VulkanContext &graphicsContext,
  Core::TrackPathID id,
  const char *path) {
  auto trackID = mRefs.find(id);

  if (trackID != mRefs.end()) {
    mRefs[id]->destroy(graphicsContext);
    mRefs[id]->init(graphicsContext);
  }
}

void ResourceTracker::addTrackedPath(
  const char *path, TrackedResourceInterface *res) {
  mRefs[Core::gFileSystem->trackPath(
    (Core::MountPoint)Core::ApplicationMountPoints::Application,
    path)] = res;
}

}
