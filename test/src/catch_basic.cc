#include <cassert>
#include <iostream>

#include "catch.hpp"

#include "rediswraps.hh"
using namespace rediswraps;


TEST_CASE("cmd() returns correct Response for all basic types supported" ) {
  Connection::Ptr redis;

  REQUIRE_NOTHROW(redis.reset(new Connection()));
  REQUIRE(redis->is_connected());

  REQUIRE(redis->cmd("DBSIZE") == 0);


  SECTION("A valid lpush command, pushing 3 items of varying types") {
    std::string key  = "foo";
    std::string arg1 = "A";
    int         arg2 = 456;
    float       arg3 = 7.89;

    redis->cmd("del", key);
    redis->cmd("lpush", key, arg1, arg2, arg3);
    redis->cmd("lrange", key, 0, -1);

    REQUIRE(redis->num_responses() == 3);

    float arg3result = redis->response();
    REQUIRE(arg3result == arg3);
    REQUIRE(redis->num_responses() == 2);

    int arg2result = redis->response();
    REQUIRE(arg2result == arg2);
    REQUIRE(redis->num_responses() == 1);

    char arg1result = redis->response();
    REQUIRE(arg1result == 'A');
    REQUIRE(redis->num_responses() == 0);
  }
}

