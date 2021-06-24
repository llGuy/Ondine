#include <string>
#include <assert.h>
#include <filesystem>
#include "FileSystem.hpp"

namespace Ondine::Core {

FileSystem *gFileSystem = nullptr;

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

}
