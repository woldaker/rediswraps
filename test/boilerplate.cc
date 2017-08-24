/*
 * Copy into src/
*/

#include <cassert>
#include <iostream>
#include <memory>

#ifdef REDISWRAPS_BENCHMARK
#include <boost/timer/timer.hpp>
#endif


#include "rediswraps.hh"
using RedisConnection = rediswraps::Connection;

//

thread_local std::unique_ptr<RedisConnection> redis;

int main(int const argc, char const *argv[]) {
  redis = new RedisConection();

  //

  return EXIT_SUCCESS;
}

