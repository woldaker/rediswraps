# Just comment this line out or delete the value on the right side of the assignment
#   for production build
HIREDIS_CPP_DEBUG :=
BENCHMARK_EXE_TIME := true

OPTIMIZATION_FLAGS := $(if $(HIREDIS_CPP_DEBUG),-ggdb3 -O0 -rdynamic -DHIREDIS_CPP_DEBUG,-O2)

COMPILE := g++ -std=c++11 -Wall -Wextra -Wfatal-errors -pedantic $(OPTIMIZATION_FLAGS) -iquote.. -I/usr/local/include -L/usr/local/lib

# the hiredis library needs warnings ignored
HIREDIS_FLAGS := -w

# the boost libs we need to link
BOOST_LIBS := $(if $(BENCHMARK_EXE_TIME),timer)

LDFLAGS := -lhiredis $(addprefix -lboost_,$(BOOST_LIBS))

test : test.cc ../hiredis_cpp.hh
	@$(COMPILE) $(HIREDIS_FLAGS) $< -o $@ $(LDFLAGS)

