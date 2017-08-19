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
    redis->Cmd("flushall");

    // Add new command 'pdel' via Lua script
    redis->load_lua_script("pdel.lua", "pdel");

    // Configure test of pdel
    auto test_pdel = [&](std::string const &pattern, int const keys_deleted_expected) -> void {
      // reset some test keys
      redis->Cmd("set", "abc", 1);
      redis->Cmd("set", "abcd", 2);
      redis->Cmd("set", "bbcd", 3);

      int keys_deleted = redis->Cmd<int>("pdel", pattern);
      assert(keys_deleted == keys_deleted_expected);
    };

    // Try it out:
    // c* should delete none
    test_pdel("c*", 0);
    std::cout << "Keys remaining after issuing command 'pdel \"c*\" :\n";
    redis->Cmd<Stash>("keys", "*");
    redis->print_responses();
    // bbc* should delete 1
    test_pdel("bbc*", 1);
    std::cout << "Keys remaining after issuing command 'pdel \"bbc*\" :\n";
    redis->Cmd<Stash>("keys", "*");
    redis->print_responses();
    // abc* should delete 2
    test_pdel("abc*", 2);
    std::cout << "Keys remaining after issuing command 'pdel \"abc*\" :\n";
    redis->Cmd<Stash>("keys", "*");
    redis->print_responses();
    // *bc* should delete all 3
    test_pdel("*bc*", 3);
    std::cout << "Keys remaining after issuing command 'pdel \"*bc*\" :\n";
    redis->Cmd<Stash>("keys", "*");
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

