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
BENCHMARK ?= 1
DEBUG     ?= 1
NOEXCEPT  ?= 0
###
### CONFIG }}}

# Make 0 values for DEBUG and BENCHMARK into empty strings for ease of use in Makefiles
override BENCHMARK := $(subst 0,,$(BENCHMARK))
override DEBUG     := $(subst 0,,$(DEBUG))
override NOEXCEPT  := $(subst 0,,$(NOEXCEPT))
export DEBUG

# Global compiler options
ifneq ($(findstring environment,$(origin QUIET)),)
$(error Do not set environment variable QUIET directly.  Use variable DEBUG instead.)
endif

export QUIET := $(if $(DEBUG),,@)


# Global Directories
export REDISWRAPS_ROOT := $(CURDIR)

INC_DIR  := $(REDISWRAPS_ROOT)/inc
TEST_DIR := $(REDISWRAPS_ROOT)/test

CFLAGS_INC_DIRS  := $(INC_DIR_HIREDIS) $(if $(subst $(INC_DIR_HIREDIS),,$(INC_DIR_BOOST)),$(INC_DIR_BOOST))
LDFLAGS_LIB_DIRS := $(LIB_DIR_HIREDIS) $(if $(subst $(LIB_DIR_HIREDIS),,$(LIB_DIR_BOOST)),$(LIB_DIR_BOOST))
LDFLAGS_LIBS     := hiredis $(if $(subst 0,,$(BENCHMARK)),boost_timer)

override CXX      ?= g++
# Add compiler -D flags depending on config variables
override CPPFLAGS ?= $(addprefix -D,$(if $(DEBUG),REDISWRAPS_DEBUG $(if $(BENCHMARK),REDISWRAPS_BENCHMARK)))
override CFLAGS   ?= -Wall -Wextra -Wfatal-errors -pedantic -pipe -march=native
# Necessary due to the hiredis library spitting out ugly warnings in combination with
#   the strict error checking flags I use
override CFLAGS   += -w
override CFLAGS   += $(if $(DEBUG),-ggdb3 -O0 -rdynamic,-O2)
override CPPFLAGS += -iquote $(INC_DIR) $(addprefix -I,$(CFLAGS_INC_DIRS))
override CXXFLAGS ?= -std=c++11
override LDFLAGS  ?= $(addprefix -L,$(LDFLAGS_LIB_DIRS)) $(addprefix -l,$(LDFLAGS_LIBS))

export CXX CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

.SUFFIXES : .cpp .cc .hpp .hh

# Make settings
export .DEFAULT_GOAL ?= all


ifeq ($(DEBUG),)
.SILENT:
endif


define PRECOMPILE_CLEAR
@clear
@for i in {0..4}; do echo; done
@echo '  ----------------------------------- make -----------------------------------  '
@clear
@echo
endef
export PRECOMPILE_CLEAR

# Pass all unknown targets directly to submake
% ::
	$(QUIET) $(MAKE) -C $(TEST_DIR) $@


