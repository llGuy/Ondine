#include "yona_utils.hpp"
#include "yona_client.hpp"
#include "yona_memory.hpp"

namespace Yona {

int entry(int argc, char **argv) {
  /* Make sure to also write FL allocator at some point */
  gLinearAllocator = Yona::flAlloc<Yona::LinearAllocator>(megabytes(10));
  gLinearAllocator->init();

  Yona::Application *client = Yona::flAlloc<Yona::Client>();
  client->run();
  return 0;
}

}

int main(int argc, char **argv) {
  return Yona::entry(argc, argv);
}
