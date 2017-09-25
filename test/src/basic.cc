#include <iostream>

#include "rediswraps.hh"
using namespace rediswraps;
thread_local Connection::Ptr redis;

#include <boost/assert.hpp>


int main(int const argc, char const *argv[]) {
  try {
    redis.reset(new Connection());

    BOOST_VERIFY_MSG(
      redis->cmd("DBSIZE") == 0,
      "RedisWraps tests will not run against existing Redis data.\n"
      "  Either backup and flush this db or spawn a new instance."
    );

    // Add new command 'pointless' from Lua script, which delegates to the 'ECHO' command to tell you how pointless it is.
    redis->load_script("pointless", "test/lua/pointless.lua");
    redis->cmd("pointless");

    while (redis->has_response()) {
      std::cout << redis->response() << "\n";
    }
    std::cout << std::endl;


    // Push some items onto a list and pull them back off into variables of various types.
    int   one, two;
    float three_point_four;

    redis->cmd("rpush",  "foo", 1, "2", "3.4");
    redis->cmd("lrange", "foo", 0, -1);
    BOOST_VERIFY(redis->num_responses() == 3);

    one = redis->response();
    std::cout << "one = " << one << std::endl;
    BOOST_VERIFY(redis->num_responses() == 2);

    two = redis->response();
    std::cout << "two = " << two << std::endl;
    BOOST_VERIFY(redis->num_responses() == 1);

    three_point_four = redis->response();
    BOOST_VERIFY(redis->num_responses() == 0);

    BOOST_ASSERT(one == 1);
    BOOST_ASSERT(two == 2);
    // Allow for inaccuracies when converting string to floating point:
    BOOST_ASSERT(three_point_four == 3.4);

    redis->cmd("del", "foo");

    BOOST_VERIFY_MSG(
      redis->cmd("DBSIZE") == 0,
      "RedisWraps tests must not leave db state with any observable modifications."
    );
  }
  catch(std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Basic functionality tests passed!" << std::endl;
  return EXIT_SUCCESS;
}

