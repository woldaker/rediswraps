# RedisWraps
#####    (Beta)

[//]: # (<img src="logo.png?raw=true"/>)

A simple and intuitive C++ interface to Redis.

### Notice

This project is very young and has several features waiting to be implemented.
More info on each is marked by a **`TODO`** tag and provided in its most relevant section below.

### Prerequisites

- C++11 capable compiler
- [hiredis][hiredis_link]
+ [Boost][boost_link] headers:
    - [lexical cast][lexical_cast_link]
    - [optional][optional_link]

### How to use it

All RedisWraps code is contained in **`namespace rediswraps`**
All the functionality needed to send and receive commands to/from Redis is encapsulated in either **`class rediswraps::Connection`** or **`class rediswraps::ConnectionNoThrow`**

<TODO>

Therefore, in your source, just add:
**`#include "rediswraps.hh"`**

  and either use it directly or make an alias (as done in the [Examples](#examples)):
**`using Redis = rediswraps::Connection;`**

The build should then look something like this:
`g++ `**`-std=c++11 -I/dir/containing/boost/headers -L/dir/containing/hiredis/lib`**` your_program.cc -o YourProgram `**`-lhiredis`**

### Examples

#### Creating a Connection
```c++
using Redis = rediswraps::Connection;
std::unique_ptr<Redis> redis(new Redis("127.0.0.1", 6379));
```

> Constructor is overloaded to take the path to a socket as well.
> You could allocate it on the stack if you wish... I just prefer using std::unique\_ptr.

#### Sending commands
At the core of sending commands and receiving responses are the functions:
**`cmd<ReturnType>(...)`**
and
**`response<ReturnType>()`**

If no ReturnType is specified, the 
```
redis->cmd("


## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc

<links>
[hiredis_link]:      https://github.com/redis/hiredis                         "hiredis"
[boost_link]:        http://www.boost.org/                                    "Boost C++ libraries"
[optional_link]:     http://www.boost.org/doc/libs/release/libs/optional/     "boost::optional"
[lexical_cast_link]: http://www.boost.org/doc/libs/release/libs/lexical_cast/ "boost::lexical_cast"

