COMPILE := g++ -std=c++11 -Wall -Wextra -Wfatal-errors -pedantic -g -iquote.. -I/usr/local/include -L/usr/local/lib

# the hiredis library needs warnings ignored
HIREDIS_FLAGS := -w

test : test.cc ../hiredis_cpp.hh
	@$(COMPILE) $(HIREDIS_FLAGS) $< -o $@ -lhiredis

