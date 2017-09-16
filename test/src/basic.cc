#include <cassert>
#include <iostream>

#include "rediswraps.hh"
using namespace rediswraps;
thread_local Connection::Ptr redis;

#include "utils.hh"


int main(int const argc, char const *argv[]) {
  redis.reset(new Connection());

  ASSERT_DB_IS_EMPTY();

  return 0;
}

/*
  std::string key  = "foo";
  std::string arg1 = "A";
  int         arg2 = 456;
  float       arg3 = 7.89;

  redis->cmd("del", key);
  redis->cmd("lpush", key, arg1, arg2, arg3);
  redis->cmd("lrange", key, 0, -1);
  redis->cmd<cmd::Flag::STASH>("lrange", key, 0 -1);

  assert(redis->num_responses() == 6);

  float arg3result = redis->response();
  std::cout << "arg3result = " << arg3result << std::endl;
  assert(arg3result >= (arg3 - 0.05));
  assert(arg3result <= (arg3 + 0.05));
  assert(redis->num_responses() == 2);

  int arg2result = redis->response();
  std::cout << "arg2result = " << arg2result << std::endl;
  assert(arg2result == arg2);
  assert(redis->num_responses() == 1);

  char arg1result = redis->response();
  std::cout << "arg1result = " << arg1result << std::endl;
  assert(arg1result == 'A');
  assert(redis->num_responses() == 0);

  return EXIT_SUCCESS;
}
*/

