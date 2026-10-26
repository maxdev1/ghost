# Native host tests; deliberately independent of the cross compiler and sysroot.
ROOT := $(abspath ../..)
BUILD_DIR ?= $(CURDIR)/bin/host
SANITIZE ?= 1
GTEST_ROOT := $(ROOT)/third_party/googletest/googletest
TEST_SOURCES := $(sort $(wildcard */*_test.cpp))
TEST_BINARIES := $(addprefix $(BUILD_DIR)/,$(TEST_SOURCES:.cpp=))
LIBC_SOURCES := errno/errno.c ctype/tolower.c ctype/toupper.c stdio/vcbprintf.c \
    sys/___tls_get_addr.c stdlib/qsort.c string/memset.c string/strchr.c \
    string/strcpy.c string/strlen.c string/strncat.c string/strrchr.c
LIBC_OBJECTS := $(addprefix $(BUILD_DIR)/libc/,$(LIBC_SOURCES:.c=.o))
RENAMED_SYMBOLS := errno memset qsort strchr strcpy strlen strncat strrchr \
    tolower toupper vcbprintf
HOST_FLAGS := -O2 -g -pthread -Wall -Wextra
SANITIZER_FLAGS :=
ifeq ($(SANITIZE),1)
SANITIZER_FLAGS += -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
endif
LIBC_FLAGS := $(HOST_FLAGS) $(SANITIZER_FLAGS) -std=c11 -fno-builtin \
    -I../inc -I$(ROOT)/libapi/inc $(foreach symbol,$(RENAMED_SYMBOLS),-D$(symbol)=ghost_$(symbol)) \
    -D___tls_get_addr=ghost___tls_get_addr -D__tls_get_addr=ghost__tls_get_addr
TEST_FLAGS := $(HOST_FLAGS) $(SANITIZER_FLAGS) -std=c++17 -Isupport -I$(GTEST_ROOT)/include
GTEST_OBJECTS := $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gtest_main.o

.PHONY: all clean
all: $(BUILD_DIR)/run_tests $(TEST_BINARIES)

$(BUILD_DIR)/run_tests: support/run_tests.cpp Makefile
	@mkdir -p "$(@D)"
	$(CXX) $(HOST_FLAGS) -std=c++17 $< -o "$@"

$(BUILD_DIR)/gtest-all.o: $(GTEST_ROOT)/src/gtest-all.cc Makefile
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -I$(GTEST_ROOT) -MMD -MP -c $< -o "$@"

$(BUILD_DIR)/gtest_main.o: $(GTEST_ROOT)/src/gtest_main.cc Makefile
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -MMD -MP -c $< -o "$@"

$(LIBC_OBJECTS): $(BUILD_DIR)/libc/%.o: ../src/%.c Makefile
	@mkdir -p "$(@D)"
	$(CC) $(LIBC_FLAGS) -MMD -MP -c $< -o "$@"

# Link each executable to the production functions it exercises. The TLS test
# alone supplies g_task_get_tls, so its object must stay out of other binaries.
$(BUILD_DIR)/ctype/ctype_test: $(addprefix $(BUILD_DIR)/libc/ctype/,tolower.o toupper.o)
$(BUILD_DIR)/stdio/vcbprintf_test: $(BUILD_DIR)/libc/stdio/vcbprintf.o $(BUILD_DIR)/libc/errno/errno.o $(BUILD_DIR)/libc/string/strlen.o
$(BUILD_DIR)/stdlib/qsort_test: $(BUILD_DIR)/libc/stdlib/qsort.o
$(BUILD_DIR)/string/memset_test: $(BUILD_DIR)/libc/string/memset.o
$(BUILD_DIR)/string/strchr_test: $(addprefix $(BUILD_DIR)/libc/string/,strchr.o strrchr.o strlen.o)
$(BUILD_DIR)/string/strncat_test: $(addprefix $(BUILD_DIR)/libc/string/,strncat.o strcpy.o strlen.o)
$(BUILD_DIR)/sys/tls_test: $(BUILD_DIR)/libc/sys/___tls_get_addr.o

$(TEST_BINARIES): $(BUILD_DIR)/%: %.cpp $(GTEST_OBJECTS) Makefile
	@mkdir -p "$(@D)"
	$(CXX) $(TEST_FLAGS) -MMD -MP -MF "$@.d" $< $(filter %.o,$^) -o "$@"

clean:
	rm -rf "$(BUILD_DIR)"

-include $(GTEST_OBJECTS:.o=.d) $(LIBC_OBJECTS:.o=.d) $(addsuffix .d,$(TEST_BINARIES))
