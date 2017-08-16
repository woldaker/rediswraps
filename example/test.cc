#include <iostream>
#include <memory>

#include <boost/timer/timer.hpp>

#include "hiredis_cpp.hh"

constexpr int DEFAULT_TEST_LEN = 25;

int main(int argc, char const *argv[]) {
  boost::timer::auto_cpu_timer t;

  // IMPORTANT: ALWAYS MAKE thread_local to avoid sharing response queues.
  thread_local std::unique_ptr<hiredis_cpp::Connection> redis(new hiredis_cpp::Connection());

  int const test_len = (argc >= 2 ? hiredis_cpp::utils::convert<int>(argv[1]) : DEFAULT_TEST_LEN);

  // We will be pushing integers onto a LIFO list (stack) and pulling them all out again and
  //   printing them in different ways.

  // don't care if it succeeds or fails, and we don't want the response to stay in the queue,
  //   so use cmd<void>()
  redis->cmd<void>("del", "foobar");
  
  for (int i = 1; i <= test_len; ++i) {
    // We want errors to be automatically reported here,
    //   and DO NOT want a bunch of 'OK' messages filling up the queue,
    //   so use cmd<bool>()
    if (!redis->cmd<bool>("lpush", "foobar", i)) {
      return EXIT_FAILURE;
    }
  }
  
  // there will now be the specified number of items on the response queue.
  int const foobar_len = redis->cmd<int>("llen", "foobar");

  if (foobar_len != test_len) {
    std::cerr << "Something went wrong here: \n"
      "  foobar_len (" << foobar_len << ") != test_len (" << test_len << ")"
    << std::endl;

    return EXIT_FAILURE;
  }

  // Pop off the values we just pushed and print them as integers...

  // Two ways to do this.

  // One, fill the response queue with all the data via:
  redis->cmd<void>("lrange", "foobar", 0, -1);

  // and loop through them:
  while (redis->has_response()) {
    // Automatically print any errors and return each value as a float
    //   from the foobar list.
    std::cout << "lrange has given: " << redis->response<float>() << "\n";
  }
  std::cout << std::endl;

  // Note that this does not actually affect list foobar because we used LRANGE
  //   and therefore do not need to fill up foobar again.

  // Perform a slightly slower but I suppose slightly more readable loop.
  // This keeps only one response in the queue at a time and destroys
  //   foobar in Redis as we pop off each value.
  for (int i = 0, i_end = redis->cmd<int>("llen", "foobar"); i < i_end; ++i) {
    std::cout << "lpop gives: " << redis->cmd<int>("lpop", "foobar") << "\n";
  }
  std::cout << std::endl;

  std::cout << "Enjoy!\n" << std::endl;

  return EXIT_SUCCESS;
}

