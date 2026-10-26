# Native host tests; deliberately independent of the cross compiler and sysroot.
ROOT := $(abspath ../..)
BUILD_DIR ?= $(CURDIR)/bin/host
SANITIZE ?= 1
GTEST_ROOT := $(ROOT)/third_party/googletest/googletest
TEST_SOURCES := $(sort $(wildcard */*_test.cpp))
TEST_BINARIES := $(addprefix $(BUILD_DIR)/,$(TEST_SOURCES:.cpp=))
PRODUCTION_SOURCES := $(shell find ../src ../../libapi/inc -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \)) ../../applications/terminal/src/terminal.cpp
HOST_FLAGS := -std=c++17 -O2 -g -pthread -Wall -Wextra
TEST_FLAGS := $(HOST_FLAGS) -Wno-unused-function -I. -I$(BUILD_DIR) -I$(GTEST_ROOT)/include
ifeq ($(SANITIZE),1)
TEST_FLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
endif
GTEST_OBJECTS := $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gtest_main.o

.PHONY: all clean terminal-lifecycle
all: $(BUILD_DIR)/run_tests $(TEST_BINARIES)

# Application lifecycle checks are built explicitly and excluded from the kernel suite.
TERMINAL_TEST := $(BUILD_DIR)/terminal/lifecycle_test
terminal-lifecycle: $(BUILD_DIR)/run_tests $(TERMINAL_TEST)
$(TERMINAL_TEST): ../../applications/terminal/tests/lifecycle_test.cpp $(GTEST_OBJECTS) $(BUILD_DIR)/production/.stamp
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -MMD -MP -MF "$@.d" $< $(GTEST_OBJECTS) -o "$@"

$(BUILD_DIR)/extract_sources: support/extract_sources.cpp support/source_recipes.inc
	@mkdir -p "$(@D)"
	$(CXX) $(HOST_FLAGS) $< -o "$@"

$(BUILD_DIR)/production/.stamp: $(BUILD_DIR)/extract_sources $(PRODUCTION_SOURCES)
	@mkdir -p "$(@D)"
	"$(BUILD_DIR)/extract_sources" "$(ROOT)" "$(@D)"
	@touch "$@"

$(BUILD_DIR)/run_tests: support/run_tests.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(HOST_FLAGS) $< -o "$@"

$(BUILD_DIR)/gtest-all.o: $(GTEST_ROOT)/src/gtest-all.cc
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -I$(GTEST_ROOT) -MMD -MP -c $< -o "$@"

$(BUILD_DIR)/gtest_main.o: $(GTEST_ROOT)/src/gtest_main.cc
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -MMD -MP -c $< -o "$@"

$(TEST_BINARIES): $(BUILD_DIR)/%: %.cpp $(GTEST_OBJECTS) $(BUILD_DIR)/production/.stamp
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -MMD -MP -MF "$@.d" $< $(GTEST_OBJECTS) -o "$@"

clean:
	rm -rf "$(BUILD_DIR)"

-include $(GTEST_OBJECTS:.o=.d) $(addsuffix .d,$(TEST_BINARIES)) $(TERMINAL_TEST).d
