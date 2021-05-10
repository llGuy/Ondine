#pragma once

#include <string>
#include <stdint.h>

#include "yona_file.hpp"

namespace Yona {

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

private:
  std::string mMountPoints[MAX_MOUNT_POINTS];
};

extern FileSystem *gFileSystem;

}
