#include <cassert>
#include <iostream>
#include <memory>

#ifdef REDISWRAPS_BENCHMARK
#include <boost/timer/timer.hpp>
#endif

#include "rediswraps.hh"
using namespace rediswraps;

constexpr int DEFAULT_TEST_LEN = 1000;

int main(int argc, char const *argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [dataset_size=" << DEFAULT_TEST_LEN << "]" << std::endl;
    return EXIT_FAILURE;
  }

#ifdef REDISWRAPS_BENCHMARK
//// START PROGRAM BENCHMARK TIMER ///////////////////
  boost::timer::auto_cpu_timer BOOST_BENCHMARK_TIMER; //
//////////////////////////////////////////////////////
#endif

  thread_local std::unique_ptr<Connection> redis(new Connection("127.0.0.1", 6379));

  int const test_len = (argc >= 2 ? utils::convert<int>(argv[1]) : DEFAULT_TEST_LEN);

  try {
    // Add new command 'pdel' via Lua script
    redis->load_lua_script("test/lua/pdel.lua", "pdel");

    // Configure test of pdel
    auto test_pdel = [&](std::string const &pattern, int const keys_deleted_expected) -> void {
      // reset some test keys
      redis->cmd("set", "abRedisWrapsc", "ab prefix, c suffix");
      redis->cmd("set", "abRedisWrapscd", "ab prefix, cd suffix");
      redis->cmd("set", "bbRedisWrapscd", "bb prefix, cd suffix");
      int keys_deleted = 
#ifdef REDISWRAPS_NOEXCEPT
        *
#endif
        redis->cmd("pdel", pattern);

      assert(keys_deleted == keys_deleted_expected);
    };

    // Try it out:
    // c* : should delete none -- no keys start with 'c'
    test_pdel("c*", 0);
    std::cout << "Keys remaining after issuing command 'pdel \"c*\" :\n";
    redis->cmd("keys", "*RedisWraps*");
    std::cout << *redis << std::endl;
    // ab*c : should delete 1
    test_pdel("ab*c", 1);
    std::cout << "Keys remaining after issuing command 'pdel \"ab*c\" :\n";
    redis->cmd("keys", "*RedisWraps*");
    std::cout << *redis << std::endl;
    // *cd should delete 2
    test_pdel("*cd", 2);
    std::cout << "Keys remaining after issuing command 'pdel \"*cd\" :\n";
    redis->cmd("keys", "*RedisWraps*");
    std::cout << *redis << std::endl;
    // *bRedisWrapsc* should delete all 3
    test_pdel("*b*c*", 3);
    std::cout << "Keys remaining after issuing command 'pdel \"*bRedisWrapsc*\" :\n";
    redis->cmd("keys", "*RedisWraps*");
    std::cout << *redis << std::endl;

    // ensure foobar key doesn't exist
    redis->cmd("del", "foobar");
  
    // push some numbers onto it as a list
    for (int i = 1; i <= test_len; ++i) {
      redis->cmd("lpush", "foobar", i);
    }
  
    // there will now be the specified number of items in the list "foobar"
    assert(redis->cmd("llen", "foobar") == test_len);

    // Pop off all the values we just pushed (using Stash)
    redis->cmd("lrange", "foobar", 0, -1);

    // and loop through them:
    int i = 0;
    int foobar_value;

    while (foobar_value = redis->response()) {
      std::cout << "[" << ++i << "] => '" << foobar_value << "'\n";
    }
  }
  catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
  }

  std::cout << "\nEnjoy!\n" << std::endl;
  return EXIT_SUCCESS;
}

