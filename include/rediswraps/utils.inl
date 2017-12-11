/* utils.inl
 *   Template implementations and static definitions for utils.hh
*/


#include <fstream>  // used in ReadFile()
#include <sstream>  // used in ReadFile()

#include <boost/lexical_cast.hpp>


namespace rediswraps {
namespace utils {

// NOTE
// The reason I did not inline this tiny template function is that if the
//   Token arguments is a char* then the std::string constructor is called.
template<typename Token, typename ArgConstructibleFromString, typename Dummy>
std::string ToString(Token const &item) {
  return item;
}


template<typename Token, typename ArgNotConstructibleFromString>
std::string ToString(Token const &item) {
  std::string converted;

  if (boost::conversion::try_lexical_convert(item, converted)) {
    return converted;
  }

  return "";
}


template<typename TargetType, typename ReturnsNonVoidDefaultConstructible>
TargetType Convert(std::string const &target) {
  TargetType new_target;

  if (boost::conversion::try_lexical_convert(target, new_target)) {
    return new_target;
  }

  return TargetType();
}

} // namespace utils
} // namespace rediswraps

