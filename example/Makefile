COMPILE := g++ -std=c++11 -Wall -Wextra -Wfatal-errors -pedantic -g -iquote.. -I/usr/local/include -L/usr/local/lib

# the hiredis library needs warnings ignored
HIREDIS_FLAGS := -w

# the boost libs we need to link
BOOST_LIBS := timer

LDFLAGS := -lhiredis $(addprefix -lboost_,$(BOOST_LIBS))

test : test.cc ../hiredis_cpp.hh
	@$(COMPILE) $(HIREDIS_FLAGS) $< -o $@ $(LDFLAGS)

