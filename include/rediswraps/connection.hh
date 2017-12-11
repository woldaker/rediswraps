#ifndef REDISWRAPS_CONNECTION_HH
#define REDISWRAPS_CONNECTION_HH

#include <deque>         // Holds all the response strings from Redis
#include <memory>        // typedef for std::unique_ptr<Connection>
#include <mutex>         // for the lock around the static scripts_ map
#include <string>
#include <type_traits>   // enable_if<>...
#include <unordered_map> // Maps Lua scripts to their hash digests.
#include <utility>       // std::pair

#include <boost/optional.hpp>

extern "C" {
#include <hiredis/hiredis.h>
}

#include <rediswraps/constants.hh>
#include <rediswraps/response.hh>


namespace rediswraps {
using ResponseQueueType = std::deque<std::string>;

class Connection {
 public:
  Connection(
      std::string const &host = constants::kDefaultHost,
      int         const  port = constants::kDefaultPort,
      std::string const &name = ""
  );

  Connection(std::string const &socket, std::string const &name = "");

  ~Connection();

  friend std::ostream& operator<< (
      std::ostream &os,
      Connection const &conn
  );

  void Flush();

  bool   const  HasResponse() const noexcept;
  size_t const NumResponses() const noexcept;
  bool   const  IsConnected() const noexcept;

  std::string const name()   const noexcept;
  std::string const socket() const noexcept;
  std::string const host()   const noexcept;
  int         const port()   const noexcept;

  // Load (Lua) Script methods
  //
  // Loads a script at either a filepath or from a string into Redis with a
  //   chosen alias.
  // The Lua script must expect at least a number of arguments equal to arg
  //   "keycount" (may be and defaults to 0), all of which should be the names
  //   of Redis keys on which to operate.  Any additional arguments may follow.
  //
  // For example, if foo.lua expects one key name as its first argument, then:
  //
  //   this->LoadScript("foo", "/path/to/foo.lua", 1);
  //
  // enables the following (assuming "bar" is some key name):
  //
  //   this->Cmd("foo", "bar", 4, 1.23);
  //
  bool const LoadScriptFromString(
      std::string const &alias,
      std::string const &script_contents,
      size_t const keycount = 0,
      bool const flush_old_scripts = false
  );

  bool const LoadScriptFromFile(
      std::string const &alias,
      std::string const &filepath,
      size_t const keycount = 0,
      bool const flush_old_scripts = false
  );

  // shorter alias for the filepath version:
  bool const LoadScript(
      std::string const &alias,
      std::string const &filepath,
      size_t const keycount = 0,
      bool const flush_old_scripts = false
  );

  // Cmd()
  // Sends Redis a command.
  // The first argument is the command itself (e.g. "SETEX") and thus must be a
  //   string.  All subsequent arguments will be converted to string
  //   automatically before being sent to Redis.
  //
  // The template argument is a bitmask enum for which you can select options.
  // See enum class cmd::Flag in constants.hh for the actual definition.
  //
  // There are really only two settings:
  // 1)  kQueue or kDiscard any responses to this command
  // 2)  kFlush or kPersist any previously queued responses beforehand..
  //
  // The default options are (kQueue | kFlush) to accomodate the basic call:
  //   std::string foo = redis->Cmd(..);
  //
  //   which would need to queue the response (so it can be assigned to foo)
  //   and flush any old responses (so foo receives the value it looks like it
  //   would and not a previously queued response)
  //
  // A compile-time error will be raised if either:
  //   Both kDiscard and kQueue are set
  //   Both kPersist and kFlush are set
  //
  template<
      cmd::Flag flags = cmd::Flag::kDefault,
      typename RetType = cmd::Response,
      typename... Args
  >
  RetType Cmd(std::string const &base, Args&&... args) noexcept;

  cmd::Response Response(
      bool const pop_response = true,
      bool const from_front   = false
  );

  template<typename RetType,
      typename ReturnsAnythingButCmdResponse = typename std::enable_if<
        !std::is_same<RetType, cmd::Response>::value
      >::type
  >
  RetType Response(
      bool const pop_response = true,
      bool const from_front   = false
  );

  std::string ResponsesToString() const;
  std::string Description() const;

 private:
  bool const UsingSocket() const noexcept;
  bool const UsingHostAndPort() const noexcept;

  void Connect();
  void Disconnect() noexcept;
  void Reconnect();

  template<cmd::Flag flags>
  cmd::Response ParseReply(redisReply *&reply, bool const recursion = false);

  template<int argc>
  void FormatCmdArgs(
      std::array<char*, argc> &&arg_strings,
      int const args_index
  );

  template<int argc, typename Arg, typename... Args>
  void FormatCmdArgs(
      std::array<char*, argc> &&arg_strings,
      int const args_index,
      Arg const &arg,
      Args&&... args
  );

  template<cmd::Flag flags, typename... Args>
  cmd::Response CmdProxy(Args&&... args);

  boost::optional<std::string> socket_;
  boost::optional<std::string> host_;
  boost::optional<int>         port_;
  boost::optional<std::string> name_;

  redisContext *context_ = nullptr;
  redisReply   *reply_   = nullptr;

  // responses_ need to be mutable because Connection::Response() needs to be
  //   const.  Else this would be possible:
  // TODO
  mutable ResponseQueueType responses_ = {};

  // scripts_
  // Maps the name of the lua script to the sha hash and the # of keys the
  //   script expects.
  //
  // Made static to enable access from Connection objects in different threads.
  //
  static std::unordered_map<
      std::string,
      std::pair<std::string, size_t>
  > scripts_;

  static std::mutex scripts_lock_;
};

using Ptr = std::unique_ptr<Connection>;
} // namespace rediswraps

#include <rediswraps/connection.inl>
#endif

