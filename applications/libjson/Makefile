ifeq ($(target),windows)
	-include ../windows-mingw.mk
else
	-include ../../ghost.mk
endif

LIB := libjson.a
CXXFLAGS := -Iinc -I$(SYSROOT)/system/include -std=c++17 # -Wall -Wextra

SRC := $(shell find src -name '*.cpp')
OBJ := $(patsubst src/%.cpp,obj/%.o,$(SRC))

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(LIB)
	mkdir -p $(SYSROOT)/system/include
	mkdir -p $(SYSROOT)/system/lib
	cp -r inc/* $(SYSROOT)/system/include/
	cp $(LIB) $(SYSROOT)/system/lib/

clean:
	rm -rf obj $(LIB)

.PHONY: all install clean
