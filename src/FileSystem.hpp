#pragma once

#include <string>
#include <stdint.h>

#include "File.hpp"

namespace Ondine::Core {

constexpr uint32_t MAX_MOUNT_POINTS = 16;

using MountPoint = uint8_t;

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

private:
  std::string mMountPoints[MAX_MOUNT_POINTS];
};

extern FileSystem *gFileSystem;

}
