#include <rediswraps/utils.hh>

#include <cstdlib>    // strtol() used in Convert<bool>

#include <rediswraps/constants.hh>


namespace rediswraps {
namespace utils {

template<>
bool Convert<bool>(std::string const &target) {
#ifdef REDISWRAPS_TRIM_STRINGS
  // TODO
#endif
  return !target.empty() && (
    (target == constants::kOk) ||
    (
      (target != constants::kNil) &&
#ifdef REDISWRAPS_BOOL_STRINGS_NOCASE
      (
        (
          (target.length() == 4) &&
          (target[0] == 'T' || target[0] == 't') &&
          (target[1] == 'R' || target[1] == 'r') &&
          (target[2] == 'U' || target[2] == 'u') &&
          (target[3] == 'E' || target[3] == 'e')
        ) ||
       !(
          (target.length() == 5) &&
          (target[0] == 'F' || target[0] == 'f') &&
          (target[1] == 'A' || target[1] == 'a') &&
          (target[2] == 'L' || target[2] == 'l') &&
          (target[3] == 'S' || target[3] == 's') &&
          (target[4] == 'E' || target[4] == 'e')
        )
      ) &&
#else
#ifdef REDISWRAPS_BOOL_STRINGS
      (
        (target == "true") ||
        (target != "false")
      ) &&
#endif
#endif
      (std::strtol(target.c_str(), nullptr, 10) != 0)
    )
  );
}


std::string const ReadFile(std::string const &filepath) {
  std::ifstream input(filepath);
  std::stringstream buffer;

  buffer << input.rdbuf();
  return buffer.str();
}
} // namespace utils
} // namespace rediswraps

