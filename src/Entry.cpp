#include "Utils.hpp"
#include "Client.hpp"
#include "Memory.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Ondine {

int entry(int argc, char **argv) {
  /* Make sure to also write FL allocator at some point */
  gLinearAllocator = Ondine::flAlloc<Ondine::LinearAllocator>(megabytes(10));
  gLinearAllocator->init();
  gFileSystem = Ondine::flAlloc<Ondine::FileSystem>();

  Ondine::Application *client = Ondine::flAlloc<Ondine::Client>(argc, argv);
  client->run();
  Ondine::flFree(client);

  return 0;
}

}

int main(int argc, char **argv) {
  return Ondine::entry(argc, argv);
}
