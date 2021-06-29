#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <filesystem>
#include <unordered_map>

#include "File.hpp"
#include "Time.hpp"
#include "Event.hpp"

namespace Ondine::Core {

constexpr uint32_t MAX_MOUNT_POINTS = 16;

using MountPoint = uint8_t;
using TrackPathID = int32_t;
using FileTime = std::filesystem::file_time_type;

struct TrackedPath {
  const std::string path;
  MountPoint mountPoint;
  FileTime lastTime;
};

/* 
   File system contains basic file / directory interaction but also
   basic resource tracker (i.e. did files marked as resources get changed?)
*/
class FileSystem {
public:
  FileSystem();

  void addMountPoint(MountPoint id, const std::string &directory);

  File createFile(
    /* ID of the mount point */
    MountPoint mountPoint,
    /* Path relative to the mount point */
    const std::string &path,
    FileOpenTypeBits type);

  bool isPathValid(
    MountPoint mountPoint,
    const std::string &path);

  void makeDirectory(
    MountPoint mountPoint,
    const std::string &path);

  TrackPathID trackPath(MountPoint mountPoint, const std::string &path);

  void trackFiles(OnEventProc onEventProc);

private:
  FileTime calculateLastWriteTime(const TrackedPath &trackedPath);

private:
  std::string mMountPoints[MAX_MOUNT_POINTS];
  std::unordered_map<std::string, TrackPathID> mPathToTrackID;
  std::vector<TrackedPath> mTrackedPaths;
  TimeStamp mPrevTrackTime;
  // Seconds
  const float mTrackInterval;
};

extern FileSystem *gFileSystem;

}
