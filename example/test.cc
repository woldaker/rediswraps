#include <iostream>
#include <memory>

#include <boost/timer/timer.hpp>

#include "hiredis_cpp.hh"

constexpr int DEFAULT_TEST_LEN = 25;

int main(int argc, char const *argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [dataset_size=" << DEFAULT_TEST_LEN << "]" << std::endl;
    return EXIT_FAILURE;
  }

//// START PROGRAM BENCHMARK TIMER ///////////////////
  boost::timer::auto_cpu_timer bOoStBeNcHmArKtImEr; //
//////////////////////////////////////////////////////

  // IMPORTANT: ALWAYS MAKE thread_local to avoid sharing response queues.
  thread_local std::unique_ptr<hiredis_cpp::Connection> redis(new hiredis_cpp::Connection());

  int const test_len = (argc >= 2 ? hiredis_cpp::utils::convert<int>(argv[1]) : DEFAULT_TEST_LEN);

  // We will be pushing integers onto a LIFO list (stack) and pulling them all out again and
  //   printing them in different ways.
/*
  if (!redis->cmd("del", "foobar")) {
    return EXIT_FAILURE;
  }
  
  for (int i = 1; i <= test_len; ++i) {
    if (!redis->cmd("lpush", "foobar", i)) {
      return EXIT_FAILURE;
    }
  }
  
  // there will now be the specified number of items in the list "foobar"
  auto foobar_len = redis->cmd<int>("llen", "foobar");

  if (foobar_len && *foobar_len != test_len) {
    std::cerr << "Something went wrong here: \n"
      "  foobar_len (" << *foobar_len << ") != test_len (" << test_len << ")"
    << std::endl;

    return EXIT_FAILURE;
  }

  // Pop off the values we just pushed and print them as integers...
  if (!redis->cmd<Stash>("lrange", "foobar", 0, -1)) {
    return EXIT_FAILURE;
  }

  // and loop through them:
  while (redis->has_response()) {
    std::cout << "lrange has given: " << *redis->response<float>() << "\n";
  }
*/
  try {
    redis->Cmd("del", "foobar");
  
    for (int i = 1; i <= test_len; ++i) {
      redis->Cmd("lpush", "foobar", i);
    }
  
    // there will now be the specified number of items in the list "foobar"
    int foobar_len = redis->Cmd<int>("llen", "foobar");

    if (foobar_len != test_len) {
      std::cerr << "Something went wrong here: \n"
        "  foobar_len (" << foobar_len << ") != test_len (" << test_len << ")"
      << std::endl;

      return EXIT_FAILURE;
    }

    // Pop off the values we just pushed and print them as integers...
    redis->Cmd<Stash>("lrange", "foobar", 0, -1);

    // and loop through them:
    while (redis->has_response()) {
      std::cout << "lrange has given: " << redis->Response<float>() << "\n";
    }
  }
  catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
  }

  std::cout << "\nEnjoy!\n" << std::endl;
  return EXIT_SUCCESS;
}

