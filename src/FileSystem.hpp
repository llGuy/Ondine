#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <unordered_map>

#include "File.hpp"

namespace Ondine::Core {

constexpr uint32_t MAX_MOUNT_POINTS = 16;

using MountPoint = uint8_t;
using TrackFileID = int32_t;

/* 
   File system contains basic file / directory interaction but also
   basic resource tracker (i.e. did files marked as resources get changed?)
*/
class FileSystem {
public:
  FileSystem() = default;

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

  File &getTrackedFile(
    MountPoint mountPoint,
    const std::string &path,
    FileOpenTypeBits type);

  TrackFileID getTrackID(const std::string &path);

  void trackFiles();

private:
  std::string mMountPoints[MAX_MOUNT_POINTS];

  // Resource management
  std::unordered_map<std::string, TrackFileID> mPathToTrackID;
};

extern FileSystem *gFileSystem;

}
