/* connection.inl
 *   Template implementations and static definitions for connection.hh
*/

#include <cstring>  // strcpy() used in FormatCmdArgs()

#include <array>    // used in CmdProxy()
#include <iostream>


namespace rediswraps {


inline
void Connection::Flush() {
  this->responses_ = {};
}


inline
bool const Connection::HasResponse() const noexcept {
  return !this->responses_.empty();
}


inline
size_t const Connection::NumResponses() const noexcept {
  return this->responses_.size();
}


inline
bool const Connection::IsConnected() const noexcept {
  return !(this->context_ == nullptr || this->context_->err);
}


inline
std::string const Connection::name() const noexcept {
  return this->name_ ? *this->name_ : constants::kUnknownStr;
}


inline
std::string const Connection::socket() const noexcept {
  return this->socket_ ? *this->socket_ : constants::kUnknownStr;
}


inline
std::string const Connection::host() const noexcept {
  return this->host_ ? *this->host_ : constants::kUnknownStr;
}


inline
int const Connection::port() const noexcept {
  return (this->port_ && (*this->port_ > 0)) ?
    *this->port_ :
    constants::kUnknownInt;
}

inline 
bool const Connection::LoadScriptFromFile(
    std::string const &alias,
    std::string const &filepath,
    size_t const keycount,
    bool const flush_old_scripts
) {
  return this->LoadScriptFromString(
    alias,
    utils::ReadFile(filepath),
    keycount,
    flush_old_scripts
  );
}

// shorter alias for the filepath version:
inline
bool const Connection::LoadScript(
    std::string const &alias,
    std::string const &filepath,
    size_t const keycount,
    bool const flush_old_scripts
) {
  return this->LoadScriptFromFile(alias, filepath, keycount, flush_old_scripts);
}


template<cmd::Flag flags, typename RetType, typename... Args>
RetType Connection::Cmd(
    std::string const &base,
    Args&&... args
) noexcept {
  static_assert(
    cmd::FlagsAreLegal<flags>::value,
    "Illegal combination of cmd::Flag values."
  );

  if (cmd::FlagsFlushResponses<flags>::value) {
    this->Flush();
  }

  cmd::Response response = this->scripts_.count(base) ?
    this->CmdProxy<flags>(
      "EVALSHA",
      this->scripts_[base].first,
      this->scripts_[base].second,
      std::forward<Args>(args)...
    ) :
    this->CmdProxy<flags>(
      base,
      std::forward<Args>(args)...
    );

  return static_cast<RetType>(response);
}


template<typename RetType, typename ReturnsAnythingButCmdResponse>
RetType Connection::Response(
    bool const pop_response,
    bool const from_front
) {
  return static_cast<RetType>(
    this->Response(pop_response, from_front)
  );
}


inline
bool const Connection::UsingSocket() const noexcept {
  return !!this->socket_;
}


inline
bool const Connection::UsingHostAndPort() const noexcept {
  return (!!this->host_) && (!!this->port_);
}


template<cmd::Flag flags>
cmd::Response Connection::ParseReply(
    redisReply *&reply,
    bool const recursion
) {
  cmd::Response response;

  // There is a corner case where we never want to stash the response:
  //   when reply->type is REDIS_REPLY_ARRAY
  bool is_array_reply = false;

  if (
      reply          == nullptr ||
      this->context_ == nullptr ||
      this->context_->err
  ) {
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

      this->Reconnect();
    }
  }
  else {
    switch(reply->type) {
    case REDIS_REPLY_ERROR:
      std::cerr << reply->str << std::endl;
      response.fail();
      // break left out intentionally here.
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_STRING:
      response.set(reply->str);
      break;
    case REDIS_REPLY_INTEGER:
      response.set(reply->integer);
      break;
    case REDIS_REPLY_NIL:
      response.set(constants::kNil);
      break;
    case REDIS_REPLY_ARRAY:
      // Do not queue THIS reply... which is just to start the array
      //   unrolling and carries no actual reply data with it.
      // Recursive calls in the following for loop will not enter this
      //   section (unless they too are arrays) and will not be affected.
      is_array_reply = true;

      for (size_t i = 0; i < reply->elements; ++i) {
        cmd::Response tmp;
        if (!(tmp = this->ParseReply<flags>(reply->element[i], true))) {
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

  if (!is_array_reply && cmd::FlagsQueueResponses<flags>::value) {
    this->responses_.emplace_front(response.data_);
  }

  if (!recursion) {
    freeReplyObject(this->reply_);
  }

  return response;
}


template<int argc>
inline
void Connection::FormatCmdArgs(
    std::array<char*, argc> &&arg_strings,
    int const args_index
) {}


template<int argc, typename Arg, typename... Args>
void Connection::FormatCmdArgs(
    std::array<char*, argc> &&arg_strings,
    int const args_index,
    Arg const &arg,
    Args&&... args
) {
  this->FormatCmdArgs<argc>(
    std::forward<std::array<char*, argc>>(arg_strings),
    (args_index + 1),
    std::forward<Args>(args)...
  );

  std::string const temp(utils::ToString(arg));

  // TODO FIXME If using new and delete, they should at least
  //   appear in the same scope
  arg_strings[args_index] = new char[temp.size() + 1];
  strcpy(arg_strings[args_index], temp.c_str());
}


template<cmd::Flag flags, typename... Args>
cmd::Response Connection::CmdProxy(Args&&... args) {
  constexpr int argc = sizeof...(args);

  std::array<char*, argc> arg_strings;

  this->FormatCmdArgs<argc>(
    std::forward<std::array<char*, argc>>(arg_strings),
    0,
    std::forward<Args>(args)...
  );

  // if it fails maybe it disconnected?...
  // try once to reconnect quickly before giving up
  bool reconnection_attempted = false;

  do {
    this->reply_ = reinterpret_cast<redisReply*>(
      redisCommandArgv(
        this->context_,
        argc,
        const_cast<char const**>(arg_strings.data()),
        nullptr
      )
    );

    if (this->reply_ != nullptr) {
      break;
    }

    if (reconnection_attempted) {
      return cmd::Response(
        this->context_->err ?
          this->context_->errstr :
          "Redis reply is null and reconnection failed.",
        false
      );
    }

    this->Reconnect();
    reconnection_attempted = true;
  }
  while (this->reply_ == nullptr);

  auto response = this->ParseReply<flags>(this->reply_);

  // TODO FIXME (same issue as above)
  // use RAII instead of new and delete[] on char arrays
  for (int i = 0; i < argc; ++i) {
    delete[] arg_strings[i];
  }

  return response;
}

} // namespace rediswraps

