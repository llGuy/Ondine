#include "Utils.hpp"
#include "Client.hpp"
#include "Memory.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Ondine {

int entry(int argc, char **argv) {
  /* Make sure to also write FL allocator at some point */
  gLinearAllocator = flAlloc<LinearAllocator>(megabytes(10));
  gLinearAllocator->init();
  gFileSystem = flAlloc<FileSystem>();

  Application *client = flAlloc<Client>(argc, argv);
  client->run();
  flFree(client);

  return 0;
}

}

int main(int argc, char **argv) {
  return Ondine::entry(argc, argv);
}
