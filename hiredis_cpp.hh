#ifndef HIREDIS_CPP_HH
#define HIREDIS_CPP_HH

#include <cstring> // strcpy() used in format_cmd_args()

#include <array>         // used in cmd_proxy()
#include <iostream>
#include <fstream>       // used in read_file()
#include <sstream>       // used in read_file()
#include <string>
#include <type_traits>   // enable_if<>...
#include <unordered_map> // Maps Lua scripts to their hash digests.
#include <utility>       // std::forward()
#include <queue>         // Holds all the response strings from Redis

#include <boost/lexical_cast.hpp>

extern "C" {
#include <hiredis/hiredis.h>
}


namespace hiredis_cpp {
constexpr char const *NIL = "(nil)";
constexpr char const *OK  = "OK";

constexpr char const   *HOST_UNSPECIFIED = "";
constexpr int           PORT_UNSPECIFIED = 0;
constexpr char const *SOCKET_UNSPECIFIED = "";

// Number of chars in hash string generated by Redis when Lua
//   or other scripts are digested and stored for reuse.

constexpr size_t SCRIPT_HASH_LENGTH = 40;

// namespace hiredis_cpp::DEFAULT {{{
namespace DEFAULT {
constexpr char const *HOST = "127.0.0.1";
constexpr int         PORT = 6379;
}
// namespace hiredis_cpp::DEFAULT }}}

namespace utils {
/*
// trim() functions {{{
void ltrim(std::string &s) {
  s.erase(
    s.begin(),
    std::find_if(
      s.begin(), s.end(),
      [] (char c) {
        return !std::isspace(c);
      }
    )
  );
}

void rtrim(std::string &s) {
  s.erase(
    std::find_if(
      s.rbegin(), s.rend(),
      [] (char c) {
        return !std::isspace(c);
      }
    ).base(),
    s.end()
  );
}

void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}
// trim() functions }}}
*/

template<typename Token, typename = typename std::enable_if<std::is_constructible<std::string, Token>::value>::type>
inline std::string to_string(Token const &item) {
  return item;
}

template<typename Token, typename = typename std::enable_if<!std::is_constructible<std::string, Token>::value>::type, typename Dummy = void>
inline std::string to_string(Token const &item) {
  std::string converted;

  return boost::conversion::try_lexical_convert(item, converted) ?
    converted :
    (std::cerr << "Conversion ERROR:\n" << __PRETTY_FUNCTION__ << std::endl, "");
}

template<typename TargetType, typename = typename std::enable_if<std::is_void<TargetType>::value || std::is_default_constructible<TargetType>::value>::type>
inline TargetType convert(std::string const &target) {
  TargetType new_target;

  return boost::conversion::try_lexical_convert(target, new_target) ?
    new_target :
    (std::cerr << "Conversion ERROR:\n" << __PRETTY_FUNCTION__ << std::endl, TargetType());
}

template<>
inline void convert<void>(std::string const &target) {}

template<>
bool convert<bool>(std::string const &target) {
  //std::string target(target_original);
  //trim(target);

  return !target.empty() && (target != NIL) &&
  (
    (target == OK) ||
    // In order to make strings literally containing "true" return true, uncomment the following:
    // (target == "true") ||
    (std::strtol(target.c_str(), nullptr, 10) != 0)
    // Case-insensitive version.  e.g. also convert "TrUe" to true:
    // || (target.length() == 4 &&
    //  (target[0] == 'T' || target[0] == 't') &&
    //  (target[1] == 'R' || target[1] == 'r') &&
    //  (target[2] == 'U' || target[2] == 'u') &&
    //  (target[3] == 'E' || target[3] == 'e')
    //)
  );
}

std::string const read_file(std::string const &filepath) {
  std::ifstream input(filepath);
  std::stringstream buffer;

  buffer << input.rdbuf();
  return buffer.str();
}

} // namespace utils


class Connection {
 public:
  Connection(std::string const &host = DEFAULT::HOST, int const port = DEFAULT::PORT)
      : host_(host),
        port_(port)
  {
    this->connect();
  }

  Connection(std::string const &socket)
      : socket_(socket)
  {
    this->connect();
  }

  ~Connection() {
    this->disconnect();
  }

  inline bool const has_response() {
    return !this->responses_.empty();
  }

  inline bool const is_connected() const noexcept {
    return !(this->context_ == nullptr || this->context_->err);
  }

  // load_lua_script() {{{
  // loads lua script at path filepath into the scripts store with an alias.
  // the script will expect "keycount" number of keys.
  //
  // For example, if foo.lua expects no keys as args, then:
  //
  //   this->load_lua_script("/path/to/foo.lua", "foo");
  //
  // enables the following:
  //
  //   this->cmd("foo", "bar", 4, 1.23);
  bool const load_lua_script(std::string const &filepath, std::string const &alias, size_t const keycount = 0, bool const flush_old_scripts = false) {
    if (flush_old_scripts) {
      if (!this->cmd<bool>("SCRIPT", "FLUSH")) {
        std::cerr << "Error: couldn't flush old Lua scripts from Redis." << std::endl;
        return false;
      }
    }

    std::string const script_hash = this->cmd<std::string>("SCRIPT", "LOAD", utils::read_file(filepath));

    if (script_hash.length() != SCRIPT_HASH_LENGTH) {
      std::cerr << "Could not load Lua script into Redis at path " << filepath << std::endl;
      return false;
    }

    this->scripts_.emplace(alias, std::pair<std::string, size_t>(script_hash, keycount));
    return true;
  }
  // load_lua_script() }}}

  // cmd<>() {{{
  // NOTE: Directly returns the response value to the caller.
  //   DOES NOT leave a new response on the queue.
  //   If the command encountered an error, that error is
  //     automatically printed to stderr and NIL is returned.
  // Use cmd<void>() if you wish to have error messages printed but disregard the
  //   response and flush it from the queue.
  // Use cmd<bool>() if you expect a bool and wish to have the error message printed,
  //   disregard the response and flush it from the queue.
  template<typename ReturnType, typename... Args>
  ReturnType cmd(std::string const &base, Args&&... args) {
    this->flush();

    if (this->cmd(base, std::forward<Args>(args)...)) {
      return this->response<ReturnType>();
    }
    
    std::cerr << "hiredis-cpp ERROR: " << this->response() << std::endl;

    return utils::convert<ReturnType>(NIL);
  }
  // cmd<>() }}}

  // cmd() {{{
  // NOTE: Returns true/false if an error occurred.
  //  DOES leave one or more responses on the response queue as a side effect.
  //  If false is returned, the error message is NOT printed to stderr but instead
  //    left on the queue.  If an error occurs, only one new response will be left
  //    in the queue, even if there were successful array-type responses that happened
  //    before the error.
  template<typename... Args>
  bool cmd(std::string const &base, Args&&... args) {
    bool success = true;

    if (this->scripts_.count(base)) {
      success = this->cmd_proxy(
        "EVALSHA",
        this->scripts_[base].first,
        this->scripts_[base].second,
        std::forward<Args>(args)...
      );
    }
    else {
      success = this->cmd_proxy(
        base,
        std::forward<Args>(args)...
      );
    }

    if (!success) {
    }

    return success;
  }
  // cmd() }}}

  // response() {{{
  template<typename ReturnType = std::string>
  ReturnType response(bool const pop_response = true, bool const from_back = false) {
    // pop_response and from_back cannot both be true due to std::queue being a container adapter,
    //   with no direct access to the underlying container.  There are ways around this if you really
    //   need this functionality (e.g. use std::deque as the underlying container)
    assert(!(pop_response && from_back));

    std::string string_response(NIL);

    if (this->has_response()) {
      string_response = from_back ?
        this->responses_.back()   :
        this->responses_.front();

      if (pop_response) {
        this->responses_.pop();
      }
    }

    return utils::convert<ReturnType>(string_response);
  }
  // response() }}}

  inline int const size() const noexcept {
    return this->responses_.size();
  }

  // last_response() {{{
  template<typename ReturnType = std::string>
  ReturnType last_response() {
    return this->response<ReturnType>(false, true);
  }
  // last_response() }}}
  
  inline void flush() {
    this->responses_ = {};
  }

  // print_responses() {{{
  void print_responses(std::string const &caller_description = "") {
    std::cout << "Hiredis++ responses";

    if (!caller_description.empty()) {
      std::cout << " from " << caller_description;
    }
    std::cout << " {";

    if (this->responses_.empty()) {
      std::cout << "}" << std::endl;
      return;
    }

    std::queue<std::string> temp_queue = {};
    swap(this->responses_, temp_queue);

    for (int i = 0; !temp_queue.empty(); ++i) {
      this->responses_.emplace(temp_queue.front());
      std::cout << "\n  [" << i << "] => '" << temp_queue.front() << "'";
      temp_queue.pop();
    }

    std::cout << "\n}" << std::endl;
  }
  // print_responses() }}}

 private:
  // dis/re/connect() {{{
  void connect() {
    if (!this->is_connected()) {
      // sockets are fastest, try that first
      if (this->socket_ != SOCKET_UNSPECIFIED) {
        this->context_ = redisConnectUnix(this->socket_.c_str());
      }
      else if (this->host_ != HOST_UNSPECIFIED && this->port_ != PORT_UNSPECIFIED) {
        this->context_ = redisConnect(this->host_.c_str(), this->port_);
      }

      if (!this->is_connected()) {
        // TODO build RedisConnectError() in exception code so we can react to this differently in main()
        throw std::runtime_error(
          this->context_ == nullptr ?
            "Unknown error connecting to Redis" :
            this->context_->errstr
        );
      }
    }
  }

  void disconnect() noexcept {
    if (this->is_connected()) {
      redisFree(this->context_);
    }

    this->context_ = nullptr;
  }

  inline void reconnect() {
    this->disconnect();
    this->connect();
  }
  // dis/connect() }}}
  
  // parse_reply() {{{
  bool parse_reply(redisReply *&reply, bool const recursion = false) {
    bool success = true;

    if (reply == nullptr || this->context_ == nullptr || this->context_->err) {
      if (reply == nullptr) {
        this->responses_.emplace("Redis reply is null");
      }
      else {
        if (reply == nullptr) {
          this->responses_.emplace("Not connected to Redis.");
        }
        else {
          this->responses_.emplace(this->context_->errstr);
        }

        this->reconnect();
      }

      success = false;
    }
    else {
      std::string error_msg;

      switch(reply->type) {
      case REDIS_REPLY_ERROR:
        success = false;
      case REDIS_REPLY_STATUS:
      case REDIS_REPLY_STRING:
        this->responses_.emplace(reply->str);
        break;
      case REDIS_REPLY_INTEGER:
        this->responses_.emplace(utils::to_string(reply->integer));
        break;
      case REDIS_REPLY_NIL:
        this->responses_.emplace(NIL);
        break;
      case REDIS_REPLY_ARRAY:
        for (size_t i = 0; i < reply->elements; ++i) {
          success &= this->parse_reply(reply->element[i], true);

          if (!success) {
            if (recursion) {
              std::string error_msg(this->response());
              this->responses_.pop();
              this->responses_.emplace(error_msg);
            }

            break;
          }
        }
        break;
      default:
        success = false;
      }
    }

    if (recursion == 0) {
      freeReplyObject(this->reply_);
    }

    return success;
  }
  // parse_reply() }}}

  // format_cmd_args() {{{
  template<int argc>
  inline void format_cmd_args(
    std::array<char*, argc> &&arg_strings,
    int const args_index
  ) {}

  template<int argc, typename Arg, typename... Args>
  void format_cmd_args(
    std::array<char*, argc> &&arg_strings,
    int const args_index,
    Arg const &arg,
    Args&&... args
  ) {
    this->format_cmd_args<argc>(
      std::forward<std::array<char*, argc>>(arg_strings),
      (args_index + 1),
      std::forward<Args>(args)...
    );

    std::string const temp(utils::to_string(arg));

    arg_strings[args_index] = new char[temp.size() + 1];
    strcpy(arg_strings[args_index], temp.c_str());
  }
  // format_cmd_args() }}}

  // cmd_proxy() {{{
  template<typename... Args>
  bool cmd_proxy(Args&&... args) {
    constexpr int argc = sizeof...(args);

    std::array<char*, argc> arg_strings;

    this->format_cmd_args<argc>(
      std::forward<std::array<char*, argc>>(arg_strings),
      0,
      std::forward<Args>(args)...
    );

    // if it fails maybe it disconnected?... try once to reconnect quickly before giving up
    bool reconnection_attempted = false;
    do {
      this->reply_ = reinterpret_cast<redisReply*>(
        redisCommandArgv(this->context_, argc, const_cast<char const**>(arg_strings.data()), nullptr)
      );

      if (this->reply_ != nullptr || reconnection_attempted) {
        break;
      }
      
      std::cerr << "Redis reply is null : " << this->context_->err << ".\n"
        "Reconnecting to Redis..." << std::endl;

      this->reconnect();
      reconnection_attempted = true;
    }
    while (this->reply_ == nullptr);
    
    bool success = this->parse_reply(this->reply_);

    for (int i = 0; i < argc; ++i) {
      delete[] arg_strings[i];
    }

    return success;
  }
  // cmd_proxy() }}}

  // member variables:
  std::string const host_   =   HOST_UNSPECIFIED;
  int         const port_   =   PORT_UNSPECIFIED;
  std::string const socket_ = SOCKET_UNSPECIFIED;

  redisContext *context_ = nullptr;
  redisReply   *reply_   = nullptr;

  std::queue<std::string> responses_ = {};

  // scripts_ maps the name of the lua script to the sha hash and the # of
  //   keys the script expects
  std::unordered_map<std::string, std::pair<std::string, size_t>> scripts_ = {};
};

} // namespace hiredis_cpp

#endif

