#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "Utils.hpp"
#include "Buffer.hpp"

namespace Ondine {

enum class FileOpenType : uint32_t {
  Text = 0,
  Binary = std::fstream::binary,
  Out = std::fstream::out,
  In = std::fstream::in,
  Truncate = std::fstream::trunc,
};

using FileOpenTypeBits = std::ios_base::openmode;

DEFINE_BIT_OPS_FOR_ENUM_CLASS(
  FileOpenType, FileOpenTypeBits, std::ios_base::openmode);

class File {
private:
  /* This is the actual full path to the file */
  File(const std::string &path, FileOpenTypeBits type);

  friend class FileSystem;

public:
  ~File() = default;

  /* Returns the number of bytes read */
  Buffer readBinary();
  std::string readText();

  /* Returns the number of bytes written */
  void write(const void *buffer, size_t size);

public:
  std::string path;
  size_t size;

private:
  std::fstream mFileStream;
  FileOpenTypeBits mFileType;
};

}
