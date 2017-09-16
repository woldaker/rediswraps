# Ensure this Makefile is always executed as a submake
ifeq ($(MAKELEVEL),0)
$(error This Makefile must be run as a submake from the project root.)
endif

TEST_PREFIX := rrtest_
CATCH_BASE := CATCH_BASE

# Local Directories
BIN_DIR := $(CURDIR)/bin
SRC_DIR := $(CURDIR)/src
OBJ_DIR := $(CURDIR)/obj
INC_DIR := $(CURDIR)/inc

GITIGNORED_DIRS := $(BIN_DIR) $(OBJ_DIR)

gitignored_dirs_exist := $(shell          \
  for dir in $(GITIGNORED_DIRS);          \
  do                                      \
  	test  -d "$${dir}" >/dev/null 2>&1 || \
    mkdir -p "$${dir}" >/dev/null 2>&1;   \
	done;                                   \
	                                        \
	echo $$?                                \
)

ifneq ($(gitignored_dirs_exist),0)
$(error Failed to create the directories required for build: $(foreach dirname,$(GITIGNORED_DIRS),"$(dirname)/"))
endif

# targets variable includes all tests in src/ except the catchlib base
all_targets   := $(filter-out $(CATCH_BASE),$(patsubst $(SRC_DIR)/%.cc,%,$(wildcard $(SRC_DIR)/*.cc)))
catch_targets := $(filter-out $(CATCH_BASE),$(patsubst $(SRC_DIR)/%.cc,%,$(wildcard $(SRC_DIR)/catch_*.cc)))
targets       := $(filter-out $(catch_targets),$(all_targets))
tests         := $(addprefix $(BIN_DIR)/$(TEST_PREFIX),$(targets))
catch_tests   := $(addprefix $(BIN_DIR)/$(TEST_PREFIX),$(catch_targets))

# All targets depend on our header
% : $(INC_DIR)/rediswraps.hh


# This version of INC_DIR is test/inc, which includes more than
#   just the rediswraps.hh headers
override CPPFLAGS += -iquote $(INC_DIR)


.PHONY: all
all : $(all_targets)

.PHONY: rm_bins
rm_bins :
	$(QUIET) rm -rf $(BIN_DIR)/*

.PHONY: rm_objs
rm_objs :
	$(QUIET) rm -rf $(OBJ_DIR)/*

.PHONY: clean
clean : rm_bins rm_objs all


# all catch targets depend on the catchlib main() being made
.PHONY: $(CATCH_BASE)
$(CATCH_BASE) : $(OBJ_DIR)/$(CATCH_BASE).o

.PHONY: $(all_targets)
$(catch_targets) : $(CATCH_BASE)

$(all_targets) : $(INC_DIR)/utils.hh

$(all_targets) : % : $(BIN_DIR)/$(TEST_PREFIX)%
ifneq ($(DEBUG),)
	$(info Made target $@)
endif


$(OBJ_DIR)/$(CATCH_BASE).o : $(SRC_DIR)/$(CATCH_BASE).cc
	$(QUIET) $(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $^ -c -o $@


# final compilation
$(tests) : $(BIN_DIR)/$(TEST_PREFIX)% : $(SRC_DIR)/%.cc
	$(PRECOMPILE_CLEAR)
	$(QUIET) $(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(catch_tests) : $(BIN_DIR)/$(TEST_PREFIX)% : $(OBJ_DIR)/$(CATCH_BASE).o $(SRC_DIR)/%.cc
	$(PRECOMPILE_CLEAR)
	$(QUIET) $(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


# For debugging variables exported to this makefile or defined locally
print_% : ;
	@echo '$*$(if $(findstring undefined,$(flavor $*)), is undefined,=$($*))'


