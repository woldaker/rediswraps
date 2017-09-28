<img align="left" src="logo.png" />

# RedisWraps
### A simple and intuitive single-header C++ interface for Redis.
<br/>

## Prerequisites
- Compiler with C++11 support
- [hiredis](https://github.com/redis/hiredis)
- [Boost](http://www.boost.org/) (specifically [boost::lexical\_cast](http://www.boost.org/doc/libs/release/libs/lexical_cast/) and [boost::optional](http://www.boost.org/doc/libs/release/lib/optional/)]

## How to use it
#### Include header and create a connection

```C++
#include "rediswraps.hh"
using Redis = rediswraps::Connection;

std::unique_ptr<Redis> redis;

// Construct with ("IP", port) or ("path/to/socket")
try {
	redis.reset(new Redis("12.34.56.78", 6379));
} catch (...) {
	/* handle error */
}
```

#### Issue commands with cmd("name", args...)

Args are automatically converted to strings.
May be of any fundamental type or any type with implicit conversion to std::string defined.

```C++
struct Foo {
	operator std::string() const { return "foo"; }
};
Foo foo;

redis->cmd("mset", foo, 123, "bar", 4.56, "gaz", false);
```

#### Get responses in a few different ways:
##### Assign to variable

```C++
// key "gaz" is set to false...
int   foo = redis->cmd("get", "foo");
float bar = redis->cmd("get", "bar");
bool  gaz = redis->cmd("get", "gaz");

foo == 123;  // true
bar == 4.56; // true
gaz == true; // true!!  See note below
```

**IMPORTANT NOTE ABOUT BOOLEANS:** Assigning to boolean will not produce the boolean value of the Redis data but whether or not the command was executed correctly.
To get exactly the behavior you want, either use a combination of **auto** and the **boolean( )** or **success( )** methods...

```C++
// key "gaz" is still set to false...

auto gazval = redis->cmd("get", "gaz");
auto errval = redis->cmd("gert", "gaz");
  // prints to stderr: "ERR unknown command 'gert'"

gazval;           // true
gazval.boolean(); // false
gazval.success(); // true
errval;           // false
errval.boolean(); // false
errval.success(); // false
```

or, use **cmd( )** directly, inline as an r-val.
When used this way, **cmd( )** will produce the same error result AND-ed with the actual Redis value:

```C++
// key "gaz" still set to false..

if (redis->cmd("get", "gaz")) {} // false as expected
if (redis->cmd("gert", "gaz")) {} // false, but not for the same reason...
  // ERR unknown command 'gert'

// A concise way to get around this is using an auto variable inline:
if (auto gazval = redis->cmd("get", "gaz")) { // true
  gazval.boolean(); // false
}

if (auto gazval = redis->cmd("gert", "gaz")) { ... } // false
  // ERR unknown command 'gert'
```

##### Loop through multiple replies with response( )

```C++
redis->cmd("rpush", "mylist", 1, "2", "3.4");
redis->cmd("lrange", "mylist", 0, -1);

// WARNING: Any calls to cmd() here will destroy the results of the previous lrange call!

while (auto listval = redis->response()) {
	std::cout << listval << std::endl;
}

// Prints:
// 1
// 2
// 3.4
```

as an alternative, you may use the **has\_response()** method:

```C++
redis->cmd("lrange", "mylist", 0, -1);

while (redis->has_response()) {
	std::cout << redis->response() << std::endl;
}
```

or if you wish to top a response (and not pop it), pass false as an argument:

```C++
auto next_response_peek = redis->response(false);
```

#### Load new commands from a Lua script:

Use either **load\_script\_from\_file( )** or **load\_script( )** (the latter is an alias for the former):

```C++
redis->cmd("gert", "mylist", "foobar");
// ERR unknown command 'gert'

redis->load_script("gert", "/path/to/script.lua",
	1   // number of KEYS[] Lua will expect; default = 0.
);

redis->cmd("gert", "mylist", "foobar");   // Now it will work
```

#### or load a script inline:

Use **load\_script\_from\_string( )**:

```C++
redis->load_script_from_string("pointless", 
	"return redis.call('ECHO', 'This command is pointless!')"
);

redis->cmd("pointless");
// prints to stdout: "This command is pointless!"
```

## Build

##### The build should look something like this.  Additional changes required are in bold.
When building an object that uses it:

`g++`**`-std=c++11 -I/path/to/boost/headers`**`-c your\_obj.cc -o your\_obj.o`

When linking a binary that uses it:

`g++`**`-std=c++11 -I/path/to/boost/headers -L/dir/containing/libhiredis.so`**`your_program.cc -o YourProgram`**`-lhiredis`**

## TODO

#### This project is very young and has quite a few features that are still missing.  Here are just a few off the top of my head:

- Async calls and responses. (In dev)
- Pubsub support. (In dev)
- Much more testing needs to be written. (In dev)
- Cluster & slave support.
- Untested on Windows (I don't have a Windows machine), but doesn't use any Unix-specific headers...
- Hardcoded command methods e.g. redis->rpush(...) (Is this really a good idea?... not sure)

## Authors

* **Wesley Oldaker** - *Initial work* - [WesleyOldaker](https://github.com/woldaker)

## License

This project is licensed under the [BSD 3-clause license](LICENSE).
