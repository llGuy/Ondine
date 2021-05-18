#include <string>
#include <assert.h>
#include <filesystem>
#include "yona_filesystem.hpp"

namespace Yona {

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

FileSystem *gFileSystem = nullptr;

}
