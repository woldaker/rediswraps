
# Ensure this Makefile is always executed as a submake
ifeq ($(MAKELEVEL),0)
$(error This Makefile must be run as a submake from the project root.)
endif

# Local Directories
BIN_DIR := $(CURDIR)/bin
SRC_DIR := $(CURDIR)/src
INC_DIR := $(CURDIR)/inc

GITIGNORED_DIRS := $(BIN_DIR)

gitignored_dirs_exist := $(shell   \
  for dir in "$(GITIGNORED_DIRS)";        \
  do                                      \
  	test  -d "$${dir}" >/dev/null 2>&1 || \
    mkdir -p "$${dir}" >/dev/null 2>&1;   \
	done;                                   \
	                                        \
	echo $$?                                \
)

ifneq ($(gitignored_dirs_exist),0)
$(error Failed to create the directories required for build: $(foreach dirname,$(GITIGNORED_DIRS),"$(dirname)"))
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
	$(QUIET) rm -f $(BIN_DIR)/*

.PHONY: debug debug_all
debug debug_all : $(debug_targets)

.PHONY: noexcept noexcept_all
noexcept noexcept_all : $(noexcept_targets)

.PHONY: debug_noexcept debug_noexcept_all
debug_noexcept debug_noexcept_all : $(debug_noexcept_targets)


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
	$(QUIET) $(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


# Debug target
print_% : FORCE
	@echo '$*$(if $(findstring undefined,$(flavor $*)), is undefined,=$($*))'

FORCE : ;


# Make settings
.DEFAULT_GOAL = all

