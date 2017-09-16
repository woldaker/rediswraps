#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <cstdio>
#include <iostream>

#ifdef REDISWRAPS_BENCHMARK
#include <boost/timer/timer.hpp>
#endif

int main(int argc, char *argv[]) {
  constexpr char const *REDIS_UP_ON_LOCALHOST_CMD =
    "ps aux | grep --invert-match 'grep' | grep --word-regexp 'redis-server' > /dev/null 2>&1";

  FILE *in;
  if(!(in = popen(REDIS_UP_ON_LOCALHOST_CMD, "r"))) {
    std::cerr << "Warning: no local Redis server instance was found to be currently running." << std::endl;
  }
  pclose(in);

#ifdef REDISWRAPS_BENCHMARK
  boost::timer::auto_cpu_timer benchmark_timer;
#endif

  int result = Catch::Session().run(argc, argv);

  if (result < 0xff) {
    return result;
  }

  return 0xff;
}

