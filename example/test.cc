#include <cassert>
#include <iostream>
#include <memory>

#ifdef REDISWRAPS_BENCHMARK
#include <boost/timer/timer.hpp>
#endif

#include "rediswraps.hh"
using RedisConnection = rediswraps::Connection;


constexpr int DEFAULT_TEST_LEN = 1000;

int main(int argc, char const *argv[]) {
  if (argc > 2) {
    std::cerr << "Usage: " << argv[0] << " [dataset_size=" << DEFAULT_TEST_LEN << "]" << std::endl;
    return EXIT_FAILURE;
  }

#ifdef REDISWRAPS_BENCHMARK
//// START PROGRAM BENCHMARK TIMER ///////////////////
  boost::timer::auto_cpu_timer bOoStBeNcHmArKtImEr; //
//////////////////////////////////////////////////////
#endif

  std::cout << "NOEXCEPT = "
#ifdef REDISWRAPS_NOEXCEPT
    "true"
#else
    "false"
#endif
  << std::endl;

  thread_local std::unique_ptr<RedisConnection> redis(new RedisConnection("127.0.0.1", 6379));

  int const test_len = (argc >= 2 ? rediswraps::utils::convert<int>(argv[1]) : DEFAULT_TEST_LEN);

  try {
    // Add new command 'pdel' via Lua script
    redis->load_lua_script("pdel.lua", "pdel");

    // Configure test of pdel
    auto test_pdel = [&](std::string const &pattern, int const keys_deleted_expected) -> void {
      // reset some test keys
      redis->Cmd("set", "abRedisWrapsc", "ab prefix, c suffix");
      redis->Cmd("set", "abRedisWrapscd", "ab prefix, cd suffix");
      redis->Cmd("set", "bbRedisWrapscd", "bb prefix, cd suffix");

      int keys_deleted = redis->Cmd<int>("pdel", pattern);
      assert(keys_deleted == keys_deleted_expected);
    };

    // Try it out:
    // c* : should delete none -- no keys start with 'c'
    test_pdel("c*", 0);
    std::cout << "Keys remaining after issuing command 'pdel \"c*\" :\n";
    redis->Cmd<Stash>("keys", "*RedisWraps*");
    redis->print_responses();
    // ab*c : should delete 1
    test_pdel("ab*c", 1);
    std::cout << "Keys remaining after issuing command 'pdel \"ab*c\" :\n";
    redis->Cmd<Stash>("keys", "*RedisWraps*");
    redis->print_responses();
    // *cd should delete 2
    test_pdel("*cd", 2);
    std::cout << "Keys remaining after issuing command 'pdel \"*cd\" :\n";
    redis->Cmd<Stash>("keys", "*RedisWraps*");
    redis->print_responses();
    // *bRedisWrapsc* should delete all 3
    test_pdel("*b*c*", 3);
    std::cout << "Keys remaining after issuing command 'pdel \"*bRedisWrapsc*\" :\n";
    redis->Cmd<Stash>("keys", "*RedisWraps*");
    redis->print_responses();

    // ensure foobar key doesn't exist
    redis->Cmd("del", "foobar");
  
    // push some numbers onto it as a list
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

