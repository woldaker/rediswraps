# Ensure this Makefile pulls in global config settings and therefore is never
#   executed directly but rather as a submake
#
# ROOT_DIR should be exported from another Makefile
ROOT_DIR ?= $(CURDIR)
BIN_DIR := bin
SRC_DIR := src
INC_DIR := inc

DIRS_NOT_ADDED_TO_GIT := $(BIN_DIR)
ensure_dirs_exist := $(shell for d in "$(DIRS_NOT_ADDED_TO_GIT)"; do test -d "${d}" || mkdir -p "${d}"; done)


ifeq ($(CURDIR),$(ROOT_DIR))
$(error This Makefile must be run as a submake from directory $(abspath $(CURDIR)/..))
endif


# Targets
targets                := $(patsubst $(SRC_DIR)/%.cc,%,$(wildcard $(SRC_DIR)/*.cc))
debug_targets          := $(addprefix debug_,$(targets))
noexcept_targets       := $(addprefix noexcept_,$(targets))
debug_noexcept_targets := $(addprefix debug_noexcept_,$(targets))
tests                  := $(addprefix $(BIN_DIR)/,$(targets))


.PHONY: all clean
all clean : $(targets)

clean :
	@rm -f $(BIN_DIR)/*

.PHONY: debug_all
debug_all : $(debug_targets)

.PHONY: noexcept_all
noexcept_all : $(noexcept_targets)

.PHONY: debug_noexcept_all
debug_noexcept_all : $(debug_noexcept_targets)


.PHONY: $(targets)
$(targets) : CFLAGS += $(OPTFLAGS)
$(targets) : % : $(BIN_DIR)/%

.PHONY: $(debug_targets)
$(debug_targets) : CFLAGS += $(DEBUG_OPTFLAGS)
$(debug_targets) : debug_% : $(BIN_DIR)/%

.PHONY: $(noexcept_targets)
$(noexcept_targets) : CFLAGS += -DREDISWRAPS_NOEXCEPT $(OPTFLAGS)
$(noexcept_targets) : noexcept_% : $(BIN_DIR)/%

.PHONY: $(debug_noexcept_targets)
$(debug_noexcept_targets) : CFLAGS += -DREDISWRAPS_NOEXCEPT $(DEBUG_OPTFLAGS)
$(debug_noexcept_targets) : debug_noexcept_% : $(BIN_DIR)/%


$(tests) : $(BIN_DIR)/% : $(SRC_DIR)/%.cc
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


.DEFAULT_GOAL = all

