# NOTICE
#   DO MODIFY THE FOLLOWING VARIABLES FOR YOUR SYSTEM OR PREFERENCES
#   OR PROVIDE THEM WHEN CALLING MAKE
INC_DIR_HIREDIS ?= /usr/local/include
LIB_DIR_HIREDIS ?= /usr/local/lib
INC_DIR_BOOST   ?= $(INC_DIR_HIREDIS)
LIB_DIR_BOOST   ?= $(LIB_DIR_HIREDIS)

# build switches - value should be 0 or 1
#   If turned on, test programs will print total execution time just prior to terminating
export BENCHMARK ?= 1


# WARNING
#   DO NOT MODIFY ANYTHING PAST THIS POINT
# Global directories
export ROOT_DIR := $(CURDIR)
export  INC_DIR := $(ROOT_DIR)/inc
export TEST_DIR := $(ROOT_DIR)/test

CFLAGS_INC_DIRS  := $(INC_DIR_HIREDIS) $(if $(subst $(INC_DIR_HIREDIS),,$(INC_DIR_BOOST)),$(INC_DIR_BOOST))
LDFLAGS_LIB_DIRS := $(LIB_DIR_HIREDIS) $(if $(subst $(LIB_DIR_HIREDIS),,$(LIB_DIR_BOOST)),$(LIB_DIR_BOOST))
LDFLAGS_LIBS := hiredis $(if $(subst 0,,$(BENCHMARK)),boost_timer)

# Global compiler options
export CXX            ?= g++
export CFLAGS         ?= -iquote $(INC_DIR) $(addprefix -I,$(CFLAGS_INC_DIRS))
export CPPFLAGS       ?= -Wall -Wextra -Wfatal-errors -pedantic-errors
export CXXFLAGS       ?= -std=c++11
export OPTFLAGS       ?= -O2
export DEBUG_OPTFLAGS ?= -g3 -O0 -rdynamic -DREDISWRAPS_DEBUG
export LDFLAGS        ?= $(addprefix -L,$(LDFLAGS_LIB_DIRS)) $(addprefix -l,$(LDFLAGS_LIBS))

# Necessary due to the hiredis library spitting out ugly warnings in combination with
#   the strict error checking flags I use
CPPFLAGS += -w


# Targets
% : $(INC_DIR)/rediswraps.hh


.PHONY: all
all : tests

.PHONY: tests
tests :
	$(MAKE) -C $(TEST_DIR)


.PHONY: clean
clean : tests_clean

.PHONY: tests_clean
tests_clean :
	$(MAKE) -C $(TEST_DIR) clean


% :
	$(MAKE) -C $(TEST_DIR) $@

.DEFAULT_GOAL := all

