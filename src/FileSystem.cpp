#include <string>
#include "Log.hpp"
#include <assert.h>
#include <system_error>
#include "FileEvent.hpp"
#include "FileSystem.hpp"

namespace Ondine::Core {

FileSystem *gFileSystem = nullptr;

FileSystem::FileSystem()
  : mTrackInterval(0.1f) {
  mPrevTrackTime = getCurrentTime();
}

void FileSystem::addMountPoint(MountPoint id, const std::string &directory) {
  mMountPoints[id] = directory + 
    (char)std::filesystem::path::preferred_separator;
}

File FileSystem::createFile(
  /* ID of the mount point */
  MountPoint mountPoint,
  /* Path relative to the mount point */
  const std::string &path,
  FileOpenTypeBits type) {
  assert(mMountPoints[mountPoint].length() > 0);
  return File(mMountPoints[mountPoint] + path, type);
}

bool FileSystem::isPathValid(
  MountPoint mountPoint,
  const std::string &path) {
  return std::filesystem::exists(mMountPoints[mountPoint] + path);
}

void FileSystem::makeDirectory(
  MountPoint mountPoint,
  const std::string &path) {
  std::filesystem::create_directory(mMountPoints[mountPoint] + path);
}

TrackPathID FileSystem::trackPath(
  MountPoint mountPoint,
  const std::string &path) {
  auto trackID = mPathToTrackID.find(path);
  if (trackID == mPathToTrackID.end()) {
    TrackPathID id = mTrackedPaths.size();

    mTrackedPaths.push_back({path, mountPoint});
    mPathToTrackID.insert(std::make_pair(path, id));

    mTrackedPaths[id].lastTime = calculateLastWriteTime(mTrackedPaths[id]);

    return id;
  }
  else {
    return trackID->second;
  }
}

void FileSystem::trackFiles(OnEventProc onEventProc) {
  auto now = getCurrentTime();

  if (getTimeDifference(now, mPrevTrackTime) > mTrackInterval) {
    mPrevTrackTime = now;

    for (uint32_t i = 0; i < mTrackedPaths.size(); ++i) {
      auto &path = mTrackedPaths[i];
      TrackPathID id = i;

      auto lastWriteTime = calculateLastWriteTime(path);

      if (lastWriteTime != path.lastTime) {
        path.lastTime = lastWriteTime;

        auto *pathChanged = lnEmplaceAlloc<EventPathChanged>();
        pathChanged->path = path.path.c_str();
        pathChanged->id = id;
        onEventProc(pathChanged);
      }
    }
  }
}

FileTime FileSystem::calculateLastWriteTime(const TrackedPath &trackedPath) {
  try {
    auto newTime = std::filesystem::last_write_time(
      mMountPoints[trackedPath.mountPoint] + trackedPath.path);

    return newTime;
  }
  catch (std::filesystem::filesystem_error &error) {
    return trackedPath.lastTime;
  }
}

}
