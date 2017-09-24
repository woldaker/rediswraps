<img src="logo.png" alt="RedisWraps logo" style="float: left; margin-right: 2em;">

<br/>

# RedisWraps

<br/>

### A simple and intuitive single-header C++ interface for Redis.
 

## Prerequisites
- Compiler with C++11 support
- [hiredis](https://github.com/redis/hiredis)
+ [Boost](http://www.boost.org/) (specifically [boost::lexical cast](http://www.boost.org/doc/libs/release/libs/lexical_cast/))

## How to use it

#### Include header and create a connection

```
#include "rediswraps.hh"
using Redis = rediswraps::Connection;

std::unique_ptr<Redis> redis;

// Construct with ("IP", port) or ("path/to/socket")
try {
	redis.reset(new Redis("66.215.238.186", 6379));
} catch (...) {/* handle error */}
```

#### Issue commands with cmd("name", args...)

Args are automatically converted to strings.
May be of any fundamental type or any type with implicit conversion to std::string defined.

```
struct Foo {
	operator std::string() const { return "foo"; }
};
Foo foo;

redis->cmd("mset", foo, 123, "bar", 4.56, "gaz", true);
```

#### Get responses in a few different ways:

##### Capture at the callpoint - assigning to type
You can just assign the result of *cmd( )* to any variable that makes sense to convert from string:

```
int    fooval = redis->cmd("get", "foo");
float  barval = redis->cmd("get", "bar");
bool   gazval = redis->cmd("get", "gaz");

assert(fooval == 123);  // true
assert(barval == 4.56); // true
assert(gazval == true); // true
```

##### Capture at the callpoint - inline

```
assert(redis->cmd("get", "foo") == 123);  // true
assert(redis->cmd("get", "bar") == 4.56); // true
assert(redis->cmd("get", "gaz") == true); // true
```

Even works correctly when using the reply as a boolean:

```
assert(redis->cmd("get", "gaz")); // true
assert(redis->cmd("gett", "gaz")); // false - prints "ERR unknown command 'gett'"
redis->cmd("set", "gaz", false);
assert(!redis->cmd("get", "gaz")); // true!  It works here as well
assert(!redis->cmd("gert", "gaz")); // false - prints "ERR unknown command 'gert'"
```

##### Loop through multiple replies with response( )

```
redis->cmd("rpush", "mylist", 1, "2", "3.4");
redis->cmd("lrange", "mylist", 0, -1);

// NOTE: Any calls to cmd() here will destroy the results of the lrange call

while (auto listval = redis->response()) {
	std::cout << listval << std::endl;
}

// Prints:
// 1
// 2
// 3.4
```

#### Load new commands from a Lua script:
```
redis->cmd("mycmd", "mylist", "foobar");
// prints "ERR unknown command 'mycmd'"

redis->load_script("mycmd", "/path/to/script.lua", 1);
// 3rd arg is # of KEYS[] to expect:

redis->cmd("mycmd", "mylist", "foobar");

// or load it inline:
//   Notice that the 3rd argument defaults to zero.
redis->load_script_from_string("pointless", 
	"return redis.call('ECHO', 'pointless cmd')"
);
redis->cmd("pointless");
// prints 'pointless cmd'
```

## Build

##### The build should look something like this.  Additional changes required are in bold.
When building an object that uses it:

`g++`**`-std=c++11 -I/path/to/boost/headers`**`-c your\_obj.cc -o your\_obj.o`

When linking a binary that uses it:

`g++`**`-std=c++11 -I/path/to/boost/headers -L/dir/containing/libhiredis.so`**`your_program.cc -o YourProgram`**`-lhiredis`**

## TODO

#### This project is very young and has quite a few features that are still missing:

...

## Authors

* **Wesley Oldaker** - *Initial work* - [WesleyOldaker](https://github.com/woldaker)

## License

This project is licensed under ???

