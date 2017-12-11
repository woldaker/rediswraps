#include <rediswraps/response.hh>


namespace rediswraps {
namespace cmd {

// NOTE
// L-val version of operator bool() in .inl file
Response::operator bool() && noexcept {
  return this->success_ && utils::Convert<bool>(this->data_);
}

bool const Response::boolean() const noexcept {
  return utils::Convert<bool>(this->data_);
}

std::ostream& operator<<(std::ostream &os, Response const &response) {
  return os << response.data_;
}

} // namespace cmd
} // namespace rediswraps

