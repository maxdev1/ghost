-include ../../ghost.mk

LIB := libpci.a
LIB_SHARED := libpci.so
CXXFLAGS := -Iinc -Wall -Wextra -std=c++17
LDFLAGS := -shared

SRC := $(shell find src -name '*.cpp')
OBJ := $(patsubst src/%.cpp,obj/%.o,$(SRC))

all: $(LIB) $(LIB_SHARED)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(LIB_SHARED): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(LIB) $(LIB_SHARED)
	mkdir -p $(SYSROOT)/system/include
	mkdir -p $(SYSROOT)/system/lib
	cp -r inc/* $(SYSROOT)/system/include/
	cp $(LIB) $(SYSROOT)/system/lib/
	cp $(LIB_SHARED) $(SYSROOT)/system/lib/

clean:
	rm -rf obj $(LIB) $(LIB_SHARED)

.PHONY: all install clean
