#include "Utils.hpp"
#include "Client.hpp"
#include "Memory.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Yona {

int entry(int argc, char **argv) {
  /* Make sure to also write FL allocator at some point */
  gLinearAllocator = Yona::flAlloc<Yona::LinearAllocator>(megabytes(10));
  gLinearAllocator->init();
  gFileSystem = Yona::flAlloc<Yona::FileSystem>();

  Yona::Application *client = Yona::flAlloc<Yona::Client>(argc, argv);
  client->run();
  Yona::flFree(client);

  return 0;
}

}

int main(int argc, char **argv) {
  return Yona::entry(argc, argv);
}
