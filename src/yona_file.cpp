#include <fstream>
#include <assert.h>
#include "yona_log.hpp"
#include "yona_file.hpp"
#include "yona_memory.hpp"

namespace Yona {

File::File(const std::string &path, FileOpenTypeBits type)
  : path(path), mFileType(type) {
  mFileStream.open(path, type);

  if (mFileStream) {
    /* Get file size */
    mFileStream.seekg(0, mFileStream.end);
    size = mFileStream.tellg();
    mFileStream.seekg(0, mFileStream.beg);
  }
  else {
    LOG_ERRORV("Couldn't find file %s\n", path.c_str());
    assert(0);
  }
}

Buffer File::readBinary() {
  Buffer buffer;
  buffer.data = flAllocv<uint8_t>(size);
  buffer.size = size;

  mFileStream.read((char *)buffer.data, buffer.size);

  return buffer;
}

std::string File::readText() {
  std::string output;
  output.reserve(size);

  output.assign(
    std::istreambuf_iterator<char>(mFileStream),
    std::istreambuf_iterator<char>());

  return output;
}

void File::write(const void *buffer, size_t size) {
  mFileStream.write((char *)buffer, size);
}

}
