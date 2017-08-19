#include <cassert>
#include <iostream>
#include <memory>

#include <boost/timer/timer.hpp>

#include "hiredis_cpp.hh"

constexpr int DEFAULT_TEST_LEN = 1000;

int main(int argc, char const *argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [dataset_size=" << DEFAULT_TEST_LEN << "]" << std::endl;
    return EXIT_FAILURE;
  }

//// START PROGRAM BENCHMARK TIMER ///////////////////
  boost::timer::auto_cpu_timer bOoStBeNcHmArKtImEr; //
//////////////////////////////////////////////////////

  thread_local std::unique_ptr<hiredis_cpp::Connection> redis(new hiredis_cpp::Connection());

  int const test_len = (argc >= 2 ? hiredis_cpp::utils::convert<int>(argv[1]) : DEFAULT_TEST_LEN);

  try {
    redis->Cmd("del", "foobar");
  
    for (int i = 1; i <= test_len; ++i) {
      redis->Cmd("lpush", "foobar", i);
    }
  
    // there will now be the specified number of items in the list "foobar"
    assert(redis->Cmd<int>("llen", "foobar") == test_len);

    // Pop off all the values we just pushed (using Stash)
    redis->Cmd<Stash>("lrange", "foobar", 0, -1);

    // and loop through them:
    for (int i = 0; redis->has_response(); ++i) {
      auto fooval = redis->Response<float>();
      std::cout << "[" << i << "] => '" << fooval << "'\n";
    }
  }
  catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
  }

  std::cout << "\nEnjoy!\n" << std::endl;
  return EXIT_SUCCESS;
}

