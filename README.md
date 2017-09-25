<img src="logo.png" alt="RedisWraps logo" style="float: left; margin-right: 2em;"/><br/>
# RedisWraps
### A simple and intuitive single-header C++ interface for Redis.

## Prerequisites
- Compiler with C++11 support
- [hiredis](https://github.com/redis/hiredis)
- [Boost](http://www.boost.org/) (specifically [boost::lexical_cast](http://www.boost.org/doc/libs/release/libs/lexical_cast/))

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

redis->cmd("mset", foo, 123, "bar", 4.56, "gaz", true);
```

#### Get responses in a few different ways:

##### Capture at the callpoint - assigning to type
You can just assign the result of **cmd( )** to any variable that makes sense to convert from string:

```C++
int   fooval = redis->cmd("get", "foo");
float barval = redis->cmd("get", "bar");
bool  gazval = redis->cmd("get", "gaz");

fooval == 123;  // true
barval == 4.56; // true
gazval == true; // true
```

##### Capture at the callpoint - inline

```C++
redis->cmd("get", "foo") == 123;  // true
redis->cmd("get", "bar") == 4.56; // true
redis->cmd("get", "gaz") == true; // true
```

When using as a boolean, be careful of one caveat.
<br/>
If you wish to distinguish between an error and a valid false value, capture the value using **auto**, then call **success( )** on the result:

```C++
redis->cmd("set", "gaz", false);
assert(!redis->cmd("get", "gaz"));  // true, but not sure why...
assert(!redis->cmd("gert", "gaz")); // also true.  Prints "ERR unknown command 'gert'"

auto gazbool_false = redis->cmd("get", "gaz");
auto gazbool_error = redis->cmd("gert", "gaz");

assert(gazbool_false.success()); // true
assert(gazbool_error.success()); // false

assert(gazbool_false); // false
assert(gazbool_error); // false
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

or if you wish to top a response (and not pop it), pass false as the second parameter of **response( )**:

```C++
auto next_response_peek = redis->response(false);
```

#### Load new commands from a Lua script:
```C++
redis->cmd("mycmd", "mylist", "foobar");
// prints "ERR unknown command 'mycmd'"

redis->load_script("mycmd", "/path/to/script.lua",
	1   // number of KEYS[] Lua will expect; default = 0.
);

redis->cmd("mycmd", "mylist", "foobar");   // Now it will work
```

#### or load a script inline:

```C++
redis->load_script_from_string("pointless", 
	"return redis.call('ECHO', 'This command is pointless!')"
);

redis->cmd("pointless");
// prints "This command is pointless!"
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
- Cluster & slave support.
- Hardcoded command methods e.g. redis->rpush(...) (Is this really a good idea?... not sure)

## Authors

* **Wesley Oldaker** - *Initial work* - [WesleyOldaker](https://github.com/woldaker)

## License

This project is licensed under the [BSD 3-clause license](LICENSE).
