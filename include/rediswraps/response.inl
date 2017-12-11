/* response.inl
 *   Template implementations and static definitions for response.hh
 */

#include <rediswraps/utils.hh>


namespace rediswraps {
namespace cmd {

template<typename T>
Response::Response(T data, bool success)
  : data_(utils::ToString(data)),
  success_(success)
{}

template<typename T, typename NotBool>
Response::operator T() const noexcept {
  return utils::Convert<T>(this->data_);
}

// NOTE
// R-val version of operator bool() in .cc file
inline
Response::operator bool() const& noexcept {
  return this->success_;
}

inline
bool const& Response::success() const noexcept {
  return this->success_;
}

template<typename T>
void Response::set(T new_data) noexcept {
  this->data_ = utils::ToString(new_data);
}

inline
void Response::fail() noexcept {
  this->success_ = false;
}

// Comparison operators {{{
// operator ==
inline
bool const operator ==(Response const &left, Response const &right) {
  return (&left == &right) ||
    (
      left.success_ == right.success_ &&
      left.data_    == right.data_
    );
}

template<typename T>
bool const operator ==(Response const &response, T const &other) {
  return utils::Convert<T>(response.data_) ==
    std::is_same<T, double>::value ? float(other) : other;
}

template<typename T>
bool const operator ==(T const &other, Response const &response) {
  return utils::Convert<T>(response.data_) ==
    std::is_same<T, double>::value ? float(other) : other;
}

// operator <
inline
bool const operator <(
    Response const &left,
    Response const &right
) {
  return (&left != &right) && (left.data_ < right.data_);
}

template<typename T>
bool const operator <(Response const &response, T const &other) {
  return utils::Convert<T>(response.data_) <
    std::is_same<T, double>::value ? float(other) : other;
}

template<typename T>
bool const operator <(T const &other, Response const &response) {
  return utils::Convert<T>(response.data_) <
    std::is_same<T, double>::value ? float(other) : other;
}

// operator !=
inline
bool const operator !=(Response const &left, Response const &right) {
  return !(left == right);
}

template<typename T>
inline
bool const operator !=(Response const &response, T const &other) {
  return !(response == other);
}

template<typename T>
inline
bool const operator !=(T const &other, Response const &response) {
  return !(other == response);
}

// operator >
inline
bool const operator >(Response const &left, Response const &right) {
  return !(left == right || left < right);
}

template<typename T>
inline
bool const operator >(Response const &response, T const &other) {
  return !(response == other || response < other);
}

template<typename T>
inline
bool const operator >(T const &other, Response const &response) {
  return !(other == response || other < response);
}

// operator <=
inline
bool const operator <=(Response const &left, Response const &right) {
  return (left == right) || (left < right);
}

template<typename T>
inline
bool const operator <=(Response const &response, T const &other) {
  return (response == other) || (response < other);
}

template<typename T>
bool const operator <=(T const &other, Response const &response) {
  return (other == response) || (other < response);
}

// operator >=
inline
bool const operator >=(Response const &left, Response const &right) {
  return !(left < right);
}

template<typename T>
inline
bool const operator >=(Response const &response, T const &other) {
  return !(response < other);
}

template<typename T>
inline
bool const operator >=(T const &other, Response const &response) {
  return !(other < response);
}
// Comparison operators }}}

} // namespace cmd
} // namespace rediswraps

