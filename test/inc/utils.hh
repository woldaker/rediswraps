#ifndef REDISWRAPS_TEST_UTILS_HH
#define REDISWRAPS_TEST_UTILS_HH

#include <iostream>

#define ASSERT(expr, msg)   \
  if (!(expr)) {            \
    std::cerr       << "\n" \
      "Assertion failed:\n" \
      "  " <<  msg  << "\n" \
      "Expected : "    "\n" \
      "  "    #expr    "\n" \
      "Source   : "    "\n" \
      "  " __FILE__     ":" \
        << __LINE__ << "\n" \
    << std::endl;           \
    abort();                \
  }


#define ASSERT_DB_IS_EMPTY()   \
  ASSERT(                      \
    redis->cmd("DBSIZE") == 0, \
    "RedisWraps tests will not run against existing Redis data.\n" \
    "  Either backup and flush this db or spawn a new instance."   \
  )

#endif

