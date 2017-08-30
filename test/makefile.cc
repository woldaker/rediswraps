/* makefile.cc
 * Test that the makefile is correctly passing preprocessor directives
*/

#include <iostream>

int main(int const argc, char const *argv[]) {
  std::cout << "NOEXCEPT = "
#ifdef REDISWRAPS_NOEXCEPT
    "true"
#else
    "false"
#endif
  << std::endl;

  std::cout << "BENCHMARK = "
#ifdef REDISWRAPS_BENCHMARK
    "true"
#else
    "false"
#endif
  << std::endl;
  
  std::cout << "DEBUG = "
#ifdef REDISWRAPS_DEBUG
    "true"
#else
    "false"
#endif
  << std::endl;

  return EXIT_SUCCESS;
}

