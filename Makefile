### CONFIG {{{
###
###   NOTICE :
###     PLEASE MODIFY THE FOLLOWING VARIABLES FOR YOUR LOCAL SYSTEM AND/OR PREFERENCES,
###     EITHER HERE OR INLINE WITH MAKE'S INVOCATION (i.e. `make FOO=bar`)
###
INC_DIR_HIREDIS ?= /usr/local/include
LIB_DIR_HIREDIS ?= /usr/local/lib
INC_DIR_BOOST   ?= $(INC_DIR_HIREDIS)
LIB_DIR_BOOST   ?= $(LIB_DIR_HIREDIS)
###
### Build Switches
###
### Set values to either 0 (OFF) or 1 (ON)
###
### BENCHMARK - If on, test programs will print total execution time just prior to terminating
BENCHMARK ?= 1
###
### CONFIG }}}


# Global Directories
override REDISWRAPS_ROOT := $(CURDIR)
export REDISWRAPS_ROOT

INC_DIR  := $(REDISWRAPS_ROOT)/inc
TEST_DIR := $(REDISWRAPS_ROOT)/test

CFLAGS_INC_DIRS  := $(INC_DIR_HIREDIS) $(if $(subst $(INC_DIR_HIREDIS),,$(INC_DIR_BOOST)),$(INC_DIR_BOOST))
LDFLAGS_LIB_DIRS := $(LIB_DIR_HIREDIS) $(if $(subst $(LIB_DIR_HIREDIS),,$(LIB_DIR_BOOST)),$(LIB_DIR_BOOST))
LDFLAGS_LIBS     := hiredis $(if $(subst 0,,$(BENCHMARK)),boost_timer)


OPTFLAGS_BENCHMARK := $(if $(subst 0,,$(BENCHMARK)),-DREDISWRAPS_BENCHMARK)

# Global compiler options
override CXX            ?= g++
override CFLAGS         += -iquote $(INC_DIR) $(addprefix -I,$(CFLAGS_INC_DIRS)) $(OPTFLAGS_BENCHMARK)
override CPPFLAGS       += -Wall -Wextra -Wfatal-errors -pedantic
override CXXFLAGS       += -std=c++11
override LDFLAGS        += $(addprefix -L,$(LDFLAGS_LIB_DIRS)) $(addprefix -l,$(LDFLAGS_LIBS))

override OPTFLAGS       ?= -O2
override DEBUG_OPTFLAGS ?= -ggdb3 -O0 -rdynamic -DREDISWRAPS_DEBUG

# Necessary due to the hiredis library spitting out ugly warnings in combination with
#   the strict error checking flags I use
override CPPFLAGS += -w

export CXX CFLAGS CPPFLAGS CXXFLAGS LDFLAGS OPTFLAGS DEBUG_OPTFLAGS

# Targets
export QUIET := @

% : $(INC_DIR)/rediswraps.hh


.PHONY: all tests
all tests : % :
	$(QUIET) $(MAKE) -C $(TEST_DIR) all


# Pass all unknown targets directly to submake
% ::
	$(QUIET) $(MAKE) -C $(TEST_DIR) $@


# Make settings
.DEFAULT_GOAL := all

#.SILENT:
.SUFFIXES : .cc .hh

