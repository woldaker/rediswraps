#ifndef REDISWRAPS_RESPONSE_HH
#define REDISWRAPS_RESPONSE_HH

#include <ostream>
#include <string>
#include <type_traits>


namespace rediswraps {
class Connection;

namespace cmd {

//   Simple wrapper around std::string that adds an error check bool
class Response {
friend class rediswraps::Connection;

 public:
  Response() = default;

  template<typename T>
  Response(T data, bool success = true);

  template<typename T,
      typename NotBool = typename std::enable_if<
        !std::is_same<T, bool>::value
      >::type
  >
  operator T() const noexcept;

  friend std::ostream& operator<<(std::ostream &os, Response const &response);

  // methods to return the error code and/or the value:

  // implicit bool casts:
  // L-val version returns the same as the success() function
  // R-val version returns the same as success() && boolean()
  operator bool() const& noexcept;
  explicit operator bool() && noexcept;

  // returns the error code ANDed with the value as a boolean
  bool const  boolean() const noexcept;
  // returns true if there was not an error
  bool const& success() const noexcept;

  // Response comparison operators {{{
  // operator ==
  friend bool const operator ==(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator ==(Response const &response, T const &other);

  template<typename T>
  friend bool const operator ==(T const &other, Response const &response);

  // operator <
  friend bool const operator <(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator <(Response const &response, T const &other);

  template<typename T>
  friend bool const operator <(T const &other, Response const &response);

  // operator !=
  friend bool const operator !=(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator !=(Response const &response, T const &other);

  template<typename T>
  friend bool const operator !=(T const &other, Response const &response);

  // operator >
  friend bool const operator >(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator >(Response const &response, T const &other);

  template<typename T>
  friend bool const operator >(T const &other, Response const &response);

  // operator <=
  friend bool const operator <=(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator <=(Response const &response, T const &other);

  template<typename T>
  friend bool const operator <=(T const &other, Response const &response);

  // operator >=
  friend bool const operator >=(Response const &left, Response const &right);

  template<typename T>
  friend bool const operator >=(Response const &response, T const &other);

  template<typename T>
  friend bool const operator >=(T const &other, Response const &response);
  // Response comparison operators }}}

 private:
  std::string data_;
  bool success_ = true;

  // set() and fail() need to be used from class Connection
  friend class Connection;

  template<typename T>
  void set(T new_data) noexcept;

  void fail() noexcept;
};

} // namespace cmd
} // namespace rediswraps

// Include template implementations
#include <rediswraps/response.inl>
#endif

