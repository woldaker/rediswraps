#ifndef REDISWRAPS_UTILS_HH
#define REDISWRAPS_UTILS_HH

#include <string>
#include <type_traits>


namespace rediswraps {
namespace utils {

template<typename Token,
    typename ArgConstructibleFromString = typename std::enable_if<
      std::is_constructible<std::string, Token>::value
    >::type,
    typename Dummy = void
>
std::string ToString(Token const &item);

template<typename Token,
    typename ArgNotConstructibleFromString = typename std::enable_if<
      !std::is_constructible<std::string, Token>::value
    >::type
>
std::string ToString(Token const &item);

template<typename TargetType,
    typename ReturnsNonVoidDefaultConstructible = typename std::enable_if<
      std::is_default_constructible<TargetType>::value &&
      !std::is_void<TargetType>::value
    >::type
>
TargetType Convert(std::string const &target);

std::string const ReadFile(std::string const &filepath);

} // namespace utils
} // namespace rediswraps

#include <rediswraps/utils.inl>
#endif

