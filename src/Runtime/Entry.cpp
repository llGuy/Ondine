#include "Utils.hpp"
#include "Client.hpp"
#include "Memory.hpp"
#include "ThreadPool.hpp"
#include "FileSystem.hpp"
#include "VulkanContext.hpp"

namespace Ondine::Runtime {

int entry(int argc, char **argv) {
  /* Make sure to also write FL allocator at some point */
  Core::gLinearAllocator = flAlloc<Core::LinearAllocator>(megabytes(10));
  Core::gLinearAllocator->init();

  Core::gFileSystem = flAlloc<Core::FileSystem>();

  Core::gThreadPool = flAlloc<Core::ThreadPool>();
  Core::gThreadPool->init();

  Application *client = flAlloc<Client>(argc, argv);
  client->run();
  flFree(client);

  return 0;
}

}

int main(int argc, char **argv) {
  return Ondine::Runtime::entry(argc, argv);
}
