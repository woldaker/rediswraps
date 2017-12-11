#include <rediswraps/connection.hh>


namespace rediswraps {

// static
std::unordered_map<std::string, std::pair<std::string, size_t>>
  Connection::scripts_ = {};

// static
std::mutex Connection::scripts_lock_;


Connection::Connection(
    std::string const &host,
    int const port,
    std::string const &name
)
  : socket_(boost::none),
    host_(boost::make_optional(!host.empty(), host)),
    port_(boost::make_optional(port > 0, port)),
    name_(boost::make_optional(!name.empty(), name))
{
  this->Connect();
}


Connection::Connection(std::string const &socket, std::string const &name)
  : socket_(boost::make_optional(!socket.empty(), socket)),
    host_(boost::none),
    port_(boost::none),
    name_(boost::make_optional(!name.empty(), name))
{
  this->Connect();
}


Connection::~Connection() {
  this->Disconnect();
}


bool const Connection::LoadScriptFromString(
    std::string const &alias,
    std::string const &script_contents,
    size_t const keycount,
    bool const reload
) {
  // Fetch scripts data structure mutex
  std::lock_guard<std::mutex> scripts_lock_guard(Connection::scripts_lock_);

  if (reload) {
    if (this->Cmd("SCRIPT", "FLUSH")) {
      std::cout <<
        "Warning: The Redis script cache has been flushed due to the request "
        "for reload of script '" << alias << "'.  Any previously loaded "
        "scripts will need to be loaded again."
      << std::endl;
    }
    else {
      std::cerr <<
        "Error: Couldn't flush old Lua scripts from Redis."
      << std::endl;
    }
  }

  if (this->scripts_.count(alias)) {
    std::cerr <<
      "Warning: Script named '" << alias << "' has already been loaded into "
      "memory.  An explicit request must be issued in order to reload this "
      "script, i.e. the \"reload\" parameter must be set."
    << std::endl;

    return false;
  }

  std::string const hashval = this->Cmd("SCRIPT", "LOAD", script_contents);

  if (hashval.empty()) {
    return false;
  }

  if (hashval.length() != constants::kScriptHashLength) {
    std::cerr <<
      "Error: Could not properly load Lua script '" << alias << "' into Redis."
      "  Invalid hash length."
    << std::endl;

    return false;
  }

  this->scripts_.emplace(
    alias,
    std::pair<std::string, size_t>(
      hashval,
      keycount
    )
  );

  return true;
}


cmd::Response Connection::Response(
    bool const pop_response,
    bool const from_front
) {
  if (pop_response && from_front) {
    std::cerr <<
      "WARNING: You are popping from the front of the Redis response "
      "queue.  This is not recommended.  See RedisWraps README for more "
      "details on why this is dangerous."
    << std::endl;
  }

  if (!this->HasResponse()) {
    return cmd::Response(
      "Redis has not previously queued any further responses.",
      false
    );
  }

  if (!pop_response) {
    return from_front ?
      this->responses_.front() :
      this->responses_.back();
  }

  cmd::Response response(
    from_front ?
      this->responses_.front() :
      this->responses_.back()
  );

  if (from_front) {
    this->responses_.pop_front();
  }
  else {
    this->responses_.pop_back();
  }

  return response;
}


void Connection::Connect() {
  if (!this->IsConnected()) {
    // sockets are fastest, try that first
    if (this->UsingSocket()) {
      this->context_ = redisConnectUnix(this->socket().c_str());
    }
    else if (this->UsingHostAndPort()) {
      this->context_ = redisConnect(this->host().c_str(), this->port());
    }

    if (!this->IsConnected()) {
      throw std::runtime_error(
        this->Description() +
        (
          this->context_ == nullptr ?
            "Unknown error connecting to Redis" :
            this->context_->errstr
        )
      );
    }

    if (this->name_) {
      this->Cmd<cmd::Flag::kClear>("CLIENT", "SETNAME", this->name());
    }
  }
}


void Connection::Disconnect() noexcept {
  if (this->IsConnected()) {
    redisFree(this->context_);
  }

  this->context_ = nullptr;
}


void Connection::Reconnect() {
  this->Disconnect();
  this->Connect();
}


std::ostream& operator<< (std::ostream &os, Connection const &conn) {
  return os << conn.Description();
}


std::string Connection::ResponsesToString() const {
  std::string desc;

  if (this->HasResponse()) {
    auto tmp_queue(this->responses_);

    for (size_t i = 0; i < tmp_queue.size(); ++i) {
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


std::string Connection::Description() const {
  std::string desc("Redis Connection {");

  desc += "\nName : "; desc += this->name();

  if (this->UsingSocket()) {
    desc += "\nSocket : "; desc += this->socket();
  }
  else if (this->UsingHostAndPort()) {
    desc += "\nHost : "; desc += this->host();
    desc += "\nPort : "; desc += utils::ToString(this->port());
  }

  desc += "\n\nResponse queue : ";
  desc += this->ResponsesToString();

  desc += "\n}";
  return desc;
}

} // namespace rediswraps

