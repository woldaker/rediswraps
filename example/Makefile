# NOEXCEPT Build switch -- 0 or 1
#   0 = cmd<T>() will ignore or forward any thrown exceptions and therefore can return type T directly
#   1 = cmd<T>() will catch its own exceptions but is therefore forced to return a boost::optional<T>
NOEXCEPT ?= 1

# BENCHMARK build switch -- 0 or 1
#   1 = Program will print total execution time after terminating
BENCHMARK ?= 1


# set some compiler flags
CXX      ?= g++
CFLAGS   += -iquote.. -I/usr/local/include -L/usr/local/lib
CPPFLAGS += -Wall -Wextra -Wfatal-errors -pedantic
# the hiredis library spits out ugly warnings in combination with the strict error checking flags I use
CPPFLAGS += -w
CXXFLAGS += -std=c++11

ifneq ($(NOEXCEPT),0)
CXXFLAGS += -DREDISWRAPS_NOEXCEPT
endif

OPTIMIZATION_FLAGS       := -O2
DEBUG_OPTIMIZAITON_FLAGS := -ggdb3 -O0 -rdynamic -DREDISWRAPS_DEBUG

LIBS := hiredis $(if $(subst 0,,$(BENCHMARK)),boost_timer)
LDFLAGS := $(addprefix -l,$(LIBS))

.DEFAULT_GOAL = test

test debug : test.cc ../rediswraps.hh

test :
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(OPTIMIZATION_FLAGS) $< -o $@ $(LDFLAGS)

debug :
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(DEBUG_OPTIMIZATION_FLAGS) $< -o $@ $(LDFLAGS)

