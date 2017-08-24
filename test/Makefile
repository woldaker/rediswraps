# Ensure this Makefile is always executed as a submake
ifeq ($(MAKELEVEL),0)
$(error This Makefile must be run as a submake from the project root.)
endif

TEST_BIN_PREFIX := rediswraps_test

# Local Directories
BIN_DIR := $(CURDIR)/bin
SRC_DIR := $(CURDIR)/src
INC_DIR := $(CURDIR)/inc

GITIGNORED_DIRS := $(BIN_DIR)

gitignored_dirs_exist := $(shell          \
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
all_targets            := $(targets) $(debug_targets) $(noexcept_targets) $(debug_noexcept_targets)

tests                := $(addprefix $(BIN_DIR)/,$(addprefix $(TEST_BIN_PREFIX)_,$(targets)))
debug_tests          := $(addprefix $(BIN_DIR)/,$(addprefix $(TEST_BIN_PREFIX)_,$(debug_targets)))
noexcept_tests       := $(addprefix $(BIN_DIR)/,$(addprefix $(TEST_BIN_PREFIX)_,$(noexcept_targets)))
debug_noexcept_tests := $(addprefix $(BIN_DIR)/,$(addprefix $(TEST_BIN_PREFIX)_,$(debug_noexcept_targets)))
all_tests            := $(tests) $(debug_tests) $(noexcept_tests) $(debug_noexcept_tests)

.PHONY: all
all : $(all_targets)

.PHONY: clean rm_bins
clean : rm_bins all
rm_bins :
	$(QUIET) rm -rf $(BIN_DIR)/*

.PHONY: debug debug_all
debug debug_all : $(debug_targets)

.PHONY: noexcept noexcept_all
noexcept noexcept_all : $(noexcept_targets)

.PHONY: debug_noexcept debug_noexcept_all
debug_noexcept debug_noexcept_all : $(debug_noexcept_targets)


.PHONY: $(all_targets)
$(targets)                : CFLAGS += $(OPTFLAGS)
$(debug_targets)          : CFLAGS += $(DEBUG_OPTFLAGS)
$(noexcept_targets)       : CFLAGS += $(OPTFLAGS)       -DREDISWRAPS_NOEXCEPT
$(debug_noexcept_targets) : CFLAGS += $(DEBUG_OPTFLAGS) -DREDISWRAPS_NOEXCEPT
$(all_targets) : % : $(BIN_DIR)/$(TEST_BIN_PREFIX)_%
	$(info Made target $@)

# final compilation
$(tests)                : $(BIN_DIR)/$(TEST_BIN_PREFIX)_%                : $(SRC_DIR)/%.cc
$(debug_tests)          : $(BIN_DIR)/$(TEST_BIN_PREFIX)_debug_%          : $(SRC_DIR)/%.cc
$(noexcept_tests)       : $(BIN_DIR)/$(TEST_BIN_PREFIX)_noexcept_%       : $(SRC_DIR)/%.cc
$(debug_noexcept_tests) : $(BIN_DIR)/$(TEST_BIN_PREFIX)_debug_noexcept_% : $(SRC_DIR)/%.cc
$(all_tests) : % :
	$(QUIET) $(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


# Debug target
print_% : FORCE
	@echo '$*$(if $(findstring undefined,$(flavor $*)), is undefined,=$($*))'

FORCE : ;


# Make settings
.DEFAULT_GOAL = all

