#ifndef REDISWRAPS_HH
#define REDISWRAPS_HH

#include <cstring> // strcpy() used in format_cmd_args()

#include <array>         // used in cmd_proxy()
#include <deque>         // Holds all the response strings from Redis
#include <iostream>
#include <fstream>       // used in read_file()
#include <memory>        // typedef for std::unique_ptr<Connection>
#include <sstream>       // used in read_file()
#include <string>
#include <type_traits>   // enable_if<>...
#include <unordered_map> // Maps Lua scripts to their hash digests.
#include <utility>       // std::forward, std::pair

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

extern "C" {
#include <hiredis/hiredis.h>
}

#ifdef REDISWRAPS_DEBUG
//#include <stdexcept>

#define INLINE inline
#else
#define INLINE 
#endif

/*
// StackFrame {{{
// 
// Container for a stack of string descriptions of each point of the real stack frame and
//   a bool flag to say if 
// 
//   
class StackFrame {
 public:
  inline
  StackFrame() noexcept
      : StackFrame(CURRENT_FUNCTION_STRING, false)
  {}

  inline
  StackFrame(bool const active) noexcept
      : StackFrame(CURRENT_FUNCTION_STRING, active)
  {}

  inline
  StackFrame(std::string &&msg, bool const active = false) noexcept
      : msg_(msg), active_(active)
  {}

  template<typename T = std::string, typename = typename std::enable_if<std::is_constructible<T, std::string>::value>::type>
  operator T() const noexcept {
};
//StackFrame }}}
*/


namespace rediswraps {

namespace cmd { // {{{
// Used as a bitmask for behavior in Connection::cmd()
enum class Flag : uint8_t {
  // Flush previous responses or keep them
  FLUSH   = 0x1,
  PERSIST = 0x2,
  // Queue response or discard/ignore it
  QUEUE   = 0x4,
  DISCARD = 0x8,

  // Just another alias for DEFAULT
  NONE = 0x0,

  // PREDEFINED COMBINATIONS:
    // DEFAULT = 0x5
    // Flush old responses and queue these ones.
    //
    // For the most basic commands, this should be fine:
    //   cmd("set", "foo", 123);
    //   int foo = cmd("get", "foo");
    //
    DEFAULT = (FLUSH | QUEUE),

    // STASH = 0x6
    // Keep old responses and also queue these ones.
    //
    // Useful for situations such as this:
    //   cmd("lpush", "my_list", "foobat", "foobas", "foobar");
    //   cmd("lrange", "my_list", 0, -1);
    //   while (cmd<STASH>("rpop", "my_list") != "foobar") {...}
    //
    STASH = (PERSIST | QUEUE),

    // CLEAR = 0x9
    // Flush old responses, also ignore this one.
    //
    // Perhaps useful for commands that need a fresh queue before and afterward, or maybe you just don't
    //   care... or maybe initializing certain keys?... and whose responses can be
    //   safely ignored for some reason?...
    // Maybe for stuff like:
    //
    //   cmd<CLEAR>("select", 2);
    //
    // NOTE: Should be zero responses afterward.  Subsequent attempts to fetch a response will be in error.
    //   response() -> returns boost::none w/ error msg attached
    //
    CLEAR = (FLUSH | DISCARD),

    // VOID = 0xA
    // Keep old responses but discard these ones.
    //
    // Useful for commands that would disrupt the state of the response queue:
    //   cmd("lpush", "my_list", cmd("rpop", "other_list"));
    //   cmd("lrange", "my_list", 0, -1);
    //     You simply cannot wait and MUST delete other_list here:
    //   cmd<VOID>("del", "other_list")
    //   
    //   while(auto queued_response = response()) { do something with queued_response... }
    //
    VOID = (PERSIST | DISCARD),


  // ILLEGAL OPTIONS: contradictory
    ILLEGAL_FLUSH_OPTS = (FLUSH | PERSIST), // 0x3
    ILLEGAL_QUEUE_OPTS = (QUEUE | DISCARD), // 0xC
    // for completeness:
    ILLEGAL_FLUSH_OPTS_2 = (ILLEGAL_FLUSH_OPTS | QUEUE),   // 0x7
    ILLEGAL_FLUSH_OPTS_3 = (ILLEGAL_FLUSH_OPTS | DISCARD), // 0xB
    ILLEGAL_QUEUE_OPTS_2 = (ILLEGAL_QUEUE_OPTS | FLUSH),   // 0xD
    ILLEGAL_QUEUE_OPTS_3 = (ILLEGAL_QUEUE_OPTS | PERSIST), // 0xE
    ILLEGAL_OPTS_ALL     = (ILLEGAL_FLUSH_OPTS | ILLEGAL_QUEUE_OPTS) // 0xF
};


namespace flag { // {{{
using cmd::Flag;
using FType = std::underlying_type<Flag>::type;

template<Flag> struct IsLegal;
template<Flag> struct FlushResponses;
template<Flag> struct QueueResponses;

template<Flag T> struct IsLegal
    : std::integral_constant<bool, 
        static_cast<FType>(T) != (static_cast<FType>(T) & static_cast<FType>(Flag::ILLEGAL_FLUSH_OPTS)) &&
        static_cast<FType>(T) != (static_cast<FType>(T) & static_cast<FType>(Flag::ILLEGAL_QUEUE_OPTS))
    >
{};

template<Flag T> struct FlushResponses
    : std::integral_constant<bool, 
        !!(static_cast<FType>(T) & static_cast<FType>(Flag::FLUSH))
    >
{};

template<Flag T> struct QueueResponses
    : std::integral_constant<bool, 
        !!(static_cast<FType>(T) & static_cast<FType>(Flag::QUEUE))
      >
{};
} // namespace flag }}}

/*
// Opt {{{
class Opt {
 public:
  INLINE
  constexpr Opt(Flag const my_flags = Flag::DEFAULT)
      : flags_(
          my_flags == Flag::NONE    ||
          my_flags == Flag::FLUSH   ||
          my_flags == Flag::PERSIST ||
          my_flags == Flag::QUEUE   ||
          my_flags == Flag::DISCARD
          ?
          static_cast<Flag>(static_cast<FlagType>(my_flags) & static_cast<FlagType>(Flag::DEFAULT))
          :
          my_flags
        )
  {
    static_assert(static_cast<FlagType>(my_flags) != (static_cast<FlagType>(my_flags) | static_cast<FlagType>(Flag::ILLEGAL_FLUSH_OPTS)),
      "cmd::Opts FLUSH and PERSIST contradict each other and cannot be combined."
    );

    static_assert(static_cast<FlagType>(my_flags) != (static_cast<FlagType>(my_flags) | static_cast<FlagType>(Flag::ILLEGAL_QUEUE_OPTS)),
      "cmd::Opts QUEUE and DISCARD contradict each other and cannot be combined."
    );
  }

  INLINE
  constexpr operator FlagType() const noexcept {
    return static_cast<FlagType>(this->flags());
  }

  INLINE
  constexpr bool operator ==(Flag const other_flag) const noexcept {
    return static_cast<FlagType>(this->flags()) == other_flag;
  }

  INLINE
  constexpr bool operator !=(Flag const other_flag) const noexcept {
    return !(*this == other_flag);
  }

  INLINE
  constexpr Opt operator |(Flag const other_flag) const noexcept {
    return static_cast<FlagType>(this->flags()) | other_flag;
  }

  INLINE
  constexpr Opt operator &(Flag const other_flag) const noexcept {
    return static_cast<FlagType>(this->flags()) & other_flag;
  }

  INLINE
  constexpr Opt operator ^(Flag const other_flag) const noexcept {
    return static_cast<FlagType>(this->flags()) ^ other_flag;
  }

  INLINE
  constexpr Opt operator !() const noexcept {
    return !static_cast<FlagType>(this->flags());
  }

  INLINE
  constexpr Flag flags() const noexcept {
    return this->flags_;
  }

 private:
  Flag const flags_;
};


namespace cmdopt {
using cmd::Opt;
using cmd::Opt::Flag;

constexpr Opt<Flag::NONE>     None;
constexpr Opt<Flag::FLUSH>    Flush;
constexpr Opt<Flag::PERSIST>  Persist;
constexpr Opt<Flag::QUEUE>    Queue;
constexpr Opt<Flag::DISCARD>  Discard;
constexpr Opt<Flag::DEFAULT>  Default;
constexpr Opt<Flag::STASH>    Stash;
constexpr Opt<Flag::CLEAR>    Clear;
constexpr Opt<Flag::VOID>     Void;
}
// class Opt }}}
*/

} // namespace cmd }}}


namespace CONSTANTS { // {{{
constexpr char const *NIL = "(nil)";
constexpr char const *OK  = "OK";

constexpr char const *UNKNOWN_STR = "";
constexpr int         UNKNOWN_INT = -1;

// Number of chars in hash string generated by Redis when Lua
//   or other scripts are digested and stored for reuse.
constexpr size_t SCRIPT_HASH_LENGTH = 40;
} // namespace rediswraps::CONSTANTS }}}

namespace DEFAULT { // {{{
constexpr char const *HOST = "127.0.0.1";
constexpr int         PORT = 6379;
} // namespace rediswraps::DEFAULT }}}


// rediswraps::utils {{{
namespace utils {

template<typename Token, typename = typename std::enable_if<std::is_constructible<std::string, Token>::value>::type, typename Dummy = void>
INLINE
std::string to_string(Token const &item) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return item;
}

template<typename Token, typename = typename std::enable_if<!std::is_constructible<std::string, Token>::value>::type>
INLINE
std::string to_string(Token const &item) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  std::string converted;

  if (boost::conversion::try_lexical_convert(item, converted)) {
    return converted;
  }

  return "";
}

// MORE DEBUG FUNCTIONS {{{
#ifdef REDISWRAPS_DEBUG
/*
template<typename Arg>
std::string args_to_string(Arg const &arg) {
  return utils::to_string(arg);
}

template<typename Arg, typename... Args>
std::string args_to_string(Arg const &arg, Args&&... args) {
  std::string arg_string(utils::to_string(arg));
  arg_string += ", ";
  return arg_string += args_to_string(std::forward<Args>(args)...);
}
*/
#endif
// MORE DEBUG FUNCTIONS }}}


template<typename TargetType, typename = typename std::enable_if<std::is_default_constructible<TargetType>::value && !std::is_void<TargetType>::value>::type>
INLINE
TargetType convert(std::string const &target) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  TargetType new_target;

#ifdef REDISWRAPS_DEBUG
  std::cout << "Converting string '" << target << "' to type '" << typeid(TargetType).name() << "'" << std::endl;
#endif

  if (boost::conversion::try_lexical_convert(target, new_target)) {
    return new_target;
  }

  return TargetType();
}

template<>
INLINE
bool convert<bool>(std::string const &target) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !target.empty() && (target != CONSTANTS::NIL) && (
    (target == CONSTANTS::OK) ||
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
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  std::ifstream input(filepath);
  std::stringstream buffer;

  buffer << input.rdbuf();
  return buffer.str();
}

} // rediswraps::utils }}}


// class Response {{{
//   Simple wrapper around std::string that adds an error check bool
class Response {
  friend class Connection;

 // public {{{
 public:
  INLINE
  Response() : success_(true) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  }

  template<typename T>
  INLINE
  Response(T data, bool success = true)
      : data_(utils::to_string(data)),
        success_(success)
  {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  }

  template<typename T>
  INLINE
  operator T() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return utils::convert<T>(this->data());
  }

  // bool() -- two variants {{{
  // For L-vals
  INLINE
  operator bool() const& noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->success();
  }

  // For R-vals
  INLINE
  operator bool() && noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->success() && utils::convert<bool>(this->data());
  }
  // bool() - two variants }}}

//TODO necessary?
  INLINE
  operator void() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  }

//  INLINE
//  operator std::string() const noexcept {
//    return this->data();
//  }

  INLINE
  std::string const data() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->data_;
  }
  
  INLINE
  bool const success() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->success_;
  }
 // public }}}

 // private {{{
 private:
  std::string data_;
  bool success_;

  template<typename T>
  INLINE
  void set(T new_data) noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->data_ = utils::to_string(new_data);
  }

  INLINE
  void fail() noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->success_ = false;
  }
 // private }}}
};
// class Response }}}


class Connection { // {{{
  using ResponseQueueType = std::deque<std::string>;

// public interface {{{
 public:
  typedef std::unique_ptr<Connection> Ptr;

  // constructors & destructors {{{
  Connection(std::string const &host = DEFAULT::HOST, int const port = DEFAULT::PORT, std::string const &name = "")
      : socket_(boost::none),
        host_(boost::make_optional(!host.empty(), host)),
        port_(boost::make_optional(port > 0, port)),
        name_(boost::make_optional(!name.empty(), name))
  {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->connect();
  }

  Connection(std::string const &socket, std::string const &name = "")
      : socket_(boost::make_optional(!socket.empty(), socket)),
        host_(boost::none),
        port_(boost::none),
        name_(boost::make_optional(!name.empty(), name))
  {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->connect();
  }

  ~Connection() {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->disconnect();
  }
  // constructors & destructors }}}

  // description() {{{
  std::string description() const {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    std::string desc("RedisWraps Connection {");
    
    desc += "\nName : "; desc += this->name();

    if (this->using_socket()) {
      desc += "\nSocket : "; desc += this->socket();
    }
    else if (this->using_host_and_port()) {
      desc += "\nHost : "; desc += this->host();
      desc += "\nPort : "; desc += utils::to_string(this->port());
    }

    desc += "\n\nQueued Responses : ";
    desc += this->responses_to_string();

    desc += "\n}";
    return desc;
  }
  // description() }}}

  // small, inline methods {{{
  INLINE
  void flush() {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->responses_ = {};
  }

  INLINE
  bool const has_response() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return !this->responses_.empty();
  }

  INLINE
  size_t const num_responses() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->responses_.size();
  }

  INLINE
  bool const is_connected() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return !(this->context_ == nullptr || this->context_->err);
  }

  INLINE
  std::string name() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->name_ ? *this->name_ : CONSTANTS::UNKNOWN_STR;
  }

  INLINE
  std::string socket() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->socket_ ? *this->socket_ : CONSTANTS::UNKNOWN_STR;
  }

  INLINE
  std::string host() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->host_ ? *this->host_ : CONSTANTS::UNKNOWN_STR;
  }

  INLINE
  int port() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->port_ ? *this->port_ : CONSTANTS::UNKNOWN_INT;
  }

//  INLINE
//  int const db() const noexcept {
//    return this->db_;
//  }

//  INLINE
//  bool const is_cluster() const noexcept {
//    return this->is_cluster_;
//  }
  // small, inline methods }}}

  // load_lua_script methods {{{
  //
  // Loads a Lua script at either a filepath or from a string into Redis with a chosen alias.
  // This script must expect its first "keycount" number of arguments to be names of Redis keys.
  //
  // For example, if foo.lua expects one key name as its first argument, then:
  //
  //   this->load_lua_script("/path/to/foo.lua", "foo", 1);
  //
  // enables the following (assuming "bar" is some key name):
  //
  //   this->cmd("foo", "bar", 4, 1.23);
  //
  bool const load_lua_script_from_string(
      std::string const &script_contents,
      std::string const &alias,
      size_t const keycount = 0,
      bool const flush_old_scripts = false
  ) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    static bool okay_to_flush = true;

    if (flush_old_scripts) {
      if (okay_to_flush) {
        if (this->cmd("SCRIPT", "FLUSH")) {
          okay_to_flush = false;
        }
        else {
          std::cerr << "Warning: Couldn't flush old Lua scripts from Redis." << std::endl;
        }
      }
      else {
        std::cerr << "Warning: Scripts previously stored in Redis have already been flushed.  "
          "There is no need to do it again."
        << std::endl;
      }
    }

    std::string const script_hash = this->cmd("SCRIPT", "LOAD", script_contents);

//TODO is this right?
    if (!script_hash) {
      return false;
    }

    if (script_hash.length() != CONSTANTS::SCRIPT_HASH_LENGTH) {
      std::cerr << "Error: Could not properly load Lua script '" << alias << "' into Redis: invalid hash length." << std::endl;
      return false;
    }

    this->scripts_.emplace(
      alias,
      std::pair<std::string, size_t>(
        *script_hash,
        keycount
      )
    );

    return true;
  }

  INLINE
  bool const load_lua_script_from_file(
      std::string const &filepath,
      std::string const &alias,
      size_t const keycount = 0,
      bool const flush_old_scripts = false
  ) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->load_lua_script_from_string(utils::read_file(filepath), alias, keycount, flush_old_scripts);
  }

  // shorter alias for the filepath version:
  INLINE
  bool const load_lua_script(
    std::string const &filepath,
    std::string const &alias,
    size_t const keycount = 0,
    bool const flush_old_scripts = false
  ) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->load_lua_script_from_file(filepath, alias, keycount, flush_old_scripts);
  }
  // load_lua_script methods }}}


  // cmd() {{{
  //   Sends Redis a command.
  // The first argument is the command itself (e.g. "SETEX") and thus must be a string.
  //   All subsequent arguments will be converted to string automatically before being sent
  //   to Redis.
  //
  // The template argument is a bitmask enum for which you can select options.
  // See enum class CmdOpt above for the actual definition.
  //
  // There are really only two settings:
  // 1)  QUEUE or DISCARD any responses to this command
  // 2)  FLUSH or PERSIST any previously queued responses before issuing this command
  //
  // The default options are (QUEUE | FLUSH) to accomodate the basic command call:
  //   std::string foo = redis->cmd(..);
  //
  //   which would need to queue the response (so it can be assigned to foo)
  //   and flush any old responses (so foo receives the value it looks like it would)
  //
  // A compile-time error will be raised if either:
  //   Both DISCARD and QUEUE are set
  //   Both PERSIST and FLUSH are set
  //
  template<cmd::Flag flags = cmd::Flag::DEFAULT, typename... Args>
  INLINE
  Connection& cmd(std::string const &base, Args&&... args) noexcept {
    std::cout << "cmd() ----> " << __PRETTY_FUNCTION__ << std::endl;
    //ADD_STACKFRAME();

    static_assert(cmd::flag::IsLegal<flags>::value,
      "Illegal combination of cmd::Flag values."
    );
    
    if (cmd::flag::FlushResponses<flags>::value) {
      this->flush();
    }

    if (this->scripts_.count(base)) {
      return this->cmd_proxy<flags>(
        "EVALSHA",
        this->scripts_[base].first,
        this->scripts_[base].second,
        std::forward<Args>(args)...
      );
    }

    if (!cmd::flag::QueueResponses<flags>::value) {
      auto discard_me = this->cmd_proxy<flags>(base, std::forward<Args>(args)...);

      //TODO is this necessary? or should I just return what cmd_proxy returns regardless?
#ifdef REDISWRAPS_DEBUG
      if (!discard_me) {
        std::cerr << "Ignoring error : '" << discard_me << "'" << std::endl;
      }
#endif

      //POP_STACKFRAME();
      return *this;
    }
    
    return this->cmd_proxy<flags>(base, std::forward<Args>(args)...);
  }
  
  
  template<typename T, typename = typename std::enable_if<std::is_void<T>::value>::type, typename... Args>
  INLINE
  T cmd(std::string const &base, Args&&... args) noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    auto discarded = this->cmd<cmd::Flag::VOID>(base, std::forward<Args>(args)...);
  }
    
 /* 
  template<unsigned char opts, typename... Args>
  INLINE
  void cmd(Args&&... args) noexcept {
    auto discard_me = this->cmd<cmdopt::QUEUE | (cmdopt::PERSIST | opts)>(std::forward<Args>(args)...);
  }
  */
  // cmd() }}}


  // response() {{{
  INLINE
  Response const response(bool const pop_response = true, bool const from_front = false) const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    if (!this->has_response()) {
      return Response("The Redis response queue is empty.", false);
    }

    if (!pop_response) {
      return Response(from_front ? this->responses_.front() : this->responses_.back());
    }
    else if (from_front) {
      std::cerr << "WARNING: You are popping from the front of the Redis response queue.  "
        "This is not recommended.  See RedisWraps README."
      << std::endl;
    }

    Response response(from_front ? this->responses_.front() : this->responses_.back());

    if (from_front) {
      this->responses_.pop_front();
    }
    else {
      this->responses_.pop_back();
    }

    return response;
  }

  template<typename T, typename = typename std::enable_if<std::is_void<T>::value>::type>
  INLINE
  void response(bool const pop_response = true, bool const from_front = false) const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    auto rsp = this->response(pop_response, from_front);

#ifdef REDISWRAPS_DEBUG
    if (!rsp) {
      std::cerr << rsp.data() << std::endl;
    }
#endif
  }
  // response() }}}

  // last_response() {{{
  //   Returns the most recent Redis response.
  // Does not pop that response from the front by default.
  // Useful for debugging.
  //
  INLINE
  Response const last_response(bool const pop_response = false) const {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return this->response(pop_response, true);
  }
  // last_response() }}}


  // all_responses() {{{
  //
  // Pass in another of the same type of queue as is this->responses_ and it simply copies them all
  //   into the target queue.
  INLINE
  void get_all_responses(ResponseQueueType &target) const {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
#ifdef REDISWRAPS_DEBUG
    if (!target.empty()) {
      std::cerr << "WARNING: Copying over non-empty queue with current responses." << std::endl;
    }
#endif

    target = this->responses_;
  }

  // Pass in nothing and it will move the values from the response queue into a new queue which it returns.
  INLINE
  std::unique_ptr<ResponseQueueType> get_all_responses() {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    auto copy_of_responses = std::unique_ptr<ResponseQueueType>{new ResponseQueueType{}};

    this->get_all_responses(*copy_of_responses);
    this->flush();

    return copy_of_responses;
  }

  // stringification and operator<< {{{
  //   returns all responses in the queue at once, as one big \n-delimited string.
  // Useful for debugging.
  std::string responses_to_string() const {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    std::string desc;
    
    if (this->has_response()) {
      auto tmp_queue(this->responses_);

      for (int i = 0; i < tmp_queue.size(); ++i) {
        desc += "\n  [";
        desc += i;
        desc += "] => '";
        desc += tmp_queue.front();
        desc += "'";

        tmp_queue.pop_front();
      }
    }

    return desc;
  }
  
  friend std::ostream& operator<< (std::ostream &os, Connection const &conn);
  // stringification and operator<< }}}

  template<typename T = std::string, typename = typename std::enable_if<!std::is_void<T>::value>::type>
  INLINE
  operator T() && {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return utils::convert<T>(this->response());
  }
  
  template<typename T = std::string, typename = typename std::enable_if<!std::is_void<T>::value>::type>
  INLINE
  operator T() const& {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return utils::convert<T>(this->response(false));
  }

  INLINE
  //template<typename T = void>//, typename = typename std::enable_if<std::is_void<T>::value>::type, typename Dummy = void>
  operator void() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  }

  template<typename T>
  friend bool const operator ==(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator ==(T const &other, Connection const &conn);
  
  template<typename T>
  friend bool const operator !=(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator !=(T const &other, Connection const &conn);
  
  template<typename T>
  friend bool const operator <(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator <(T const &other, Connection const &conn);

  template<typename T>
  friend bool const operator <=(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator <=(T const &other, Connection const &conn);

  template<typename T>
  friend bool const operator >(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator >(T const &other, Connection const &conn);

  template<typename T>
  friend bool const operator >=(Connection const &conn, T const &other);

  template<typename T>
  friend bool const operator >=(T const &other, Connection const &conn);

  INLINE
  bool const operator ==(Connection const &other) const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return (this == &other) ||
      (
        utils::convert<std::string>(this->response(false)) ==
        utils::convert<std::string>(other.response(false))
      );
  }
// public interface }}}

// private {{{
 private:
  INLINE
  bool const using_socket() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return !!this->socket_;
  }

  INLINE
  bool const using_host_and_port() const noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    return (!!this->host_) && (!!this->port_);
  }

  // dis/re/connect() {{{
  void connect() {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    if (!this->is_connected()) {
      // sockets are fastest, try that first
      if (this->using_socket()) {
        this->context_ = redisConnectUnix(this->socket().c_str());
      }
      else if (this->using_host_and_port()) {
#ifdef REDISWRAPS_DEBUG
        std::cout << "host: " << this->host().c_str() << "    port: " << this->port() << std::endl;
#endif
        this->context_ = redisConnect(this->host().c_str(), this->port());
      }

      if (!this->is_connected()) {
        throw std::runtime_error(
          this->description() +
            (this->context_ == nullptr ?
              "Unknown error connecting to Redis" :
              this->context_->errstr
            )
        );
      }

      if (this->name_) {
        this->cmd<cmd::Flag::CLEAR>("CLIENT", "SETNAME", this->name());
      }
    }
  }

  INLINE
  void disconnect() noexcept {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
#ifdef REDISWRAPS_DEBUG
    std::cout << "RESPONSES @ DISCONNECTION:\n" << this->responses_to_string() << std::endl;
#endif

    if (this->is_connected()) {
      redisFree(this->context_);
    }

    this->context_ = nullptr;
  }

  INLINE
  void reconnect() {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    this->disconnect();
    this->connect();
  }
  // dis/connect() }}}
  
  // parse_reply() {{{
  template<cmd::Flag flags>
  Response parse_reply(redisReply *&reply, bool const recursion = false) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
    Response response;
    
    // There is a corner case where we never want to stash the response:
    //   when reply->type is REDIS_REPLY_ARRAY
    bool is_array_reply = false;

    if (reply == nullptr || this->context_ == nullptr || this->context_->err) {
      response.fail();

      if (reply == nullptr) {
        response.set("Redis reply is null");
      }
      else {
        if (this->context_ == nullptr) {
          response.set("Not connected to Redis");
        }
        else {
          response.set(this->context_->errstr);
        }

        this->reconnect();
      }
    }
    else {
      switch(reply->type) {
      case REDIS_REPLY_ERROR:
        response.fail();
        break;
      case REDIS_REPLY_STATUS:
      case REDIS_REPLY_STRING:
        response.set(reply->str);
        break;
      case REDIS_REPLY_INTEGER:
        response.set(reply->integer);
        break;
      case REDIS_REPLY_NIL:
        response.set(CONSTANTS::NIL);
        break;
      case REDIS_REPLY_ARRAY:
        // Do not queue THIS reply... which is just to start the array unrolling and
        //   carries no actual reply data with it.
        // Recursive calls in the following for loop will not enter this section (unless
        //   they too are arrays...) and will not be affected
        is_array_reply = true;

        for (size_t i = 0; i < reply->elements; ++i) {
          if (!this->parse_reply<flags>(reply->element[i], true)) {
            // if there is an error remove all the successful responses that were pushed
            //  onto the response queue before the error occurred.
            while (i-- > 0) {
              this->responses_.pop_back();
            }
            break;
          }
        }
        break;
      default:
        response.fail();
      }
    }

    if (!is_array_reply && cmd::flag::QueueResponses<flags>::value) {
      this->responses_.emplace_front(response.data());
    }

    if (!recursion) {
      freeReplyObject(this->reply_);
    }

    return response;
  }
  // parse_reply() }}}

  // format_cmd_args() {{{
  template<int argc>
  INLINE
  void format_cmd_args(
    std::array<char*, argc> &&arg_strings,
    int const args_index
  ) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  }

  template<int argc, typename Arg, typename... Args>
  void format_cmd_args(
    std::array<char*, argc> &&arg_strings,
    int const args_index,
    Arg const &arg,
    Args&&... args
  ) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;

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
  template<cmd::Flag flags, typename... Args>
  Connection& cmd_proxy(Args&&... args) {
    //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
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

      if (this->reply_ != nullptr) {
        break;
      }

      if (reconnection_attempted) {
        if (cmd::flag::QueueResponses<flags>::value) {
          this->responses_.emplace_front(
            this->context_->err ?
              this->context_->errstr :
              "Redis reply is null and reconnection failed.",
            false
          );
        }

        return *this;
      }

      this->reconnect();
      reconnection_attempted = true;
    }
    while (this->reply_ == nullptr);
    
    this->parse_reply<flags>(this->reply_);

    for (int i = 0; i < argc; ++i) {
      delete[] arg_strings[i];
    }

    return *this;
  }
  // cmd_proxy() }}}

  // member variables {{{
  boost::optional<std::string> socket_;
  boost::optional<std::string> host_;
  boost::optional<int>         port_;
  boost::optional<std::string> const name_;

/*
  bool is_cluster_;
  int db_;
*/

  redisContext *context_ = nullptr;
  redisReply   *reply_   = nullptr;

  mutable ResponseQueueType responses_ = {};

  // scripts_ maps the name of the lua script to the sha hash and the # of
  //   keys the script expects
  std::unordered_map<std::string, std::pair<std::string, size_t>> scripts_ = {};
  // member variables }}}
// private }}}
};
// class Connection }}}

INLINE std::ostream& operator<< (std::ostream &os, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return os << conn.description();
}

// operator ==
template<typename T>
INLINE
bool const operator ==(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  auto response = conn.response(false);
  return response && (utils::convert<T>(response.data()) == other);
}

template<typename T>
INLINE
bool const operator ==(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  auto response = conn.response(false);
  return response && (other == utils::convert<T>(response.data()));
}

// operator <
template<typename T>
INLINE
bool const operator <(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  auto response = conn.response(false);
  return response && (utils::convert<T>(response.data()) < other);
}

template<typename T>
INLINE
bool const operator <(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  auto response = conn.response(false);
  return response && (other < utils::convert<T>(response.data()));
}

// operator !=
template<typename T>
INLINE
bool const operator !=(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(conn == other);
}

template<typename T>
INLINE
bool const operator !=(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(other == conn);
}

// opeartor >
template<typename T>
INLINE
bool const operator >(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(conn < other || conn == other);
}

template<typename T>
INLINE
bool const operator >(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(other < conn || other == conn);
}

// operator <=
template<typename T>
INLINE
bool const operator <=(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(conn > other);
}

template<typename T>
INLINE
bool const operator <=(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(other > conn);
}

// operator >=
template<typename T>
INLINE
bool const operator >=(Connection const &conn, T const &other) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(conn < other);
}

template<typename T>
INLINE
bool const operator >=(T const &other, Connection const &conn) {
  //std::cout << "----> " << __PRETTY_FUNCTION__ << std::endl;
  return !(other < conn);
}

} // namespace rediswraps

//TODO get rid of this macro
//#undef INLINE

#endif

