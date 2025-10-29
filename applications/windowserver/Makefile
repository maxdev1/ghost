ifeq ($(target),windows)
	-include ../windows-mingw.mk
	LINPUT :=
	LVIDEO :=
	LPS2DRIVER :=
	EXTRA_CXXFLAGS := -I$(SYSROOT)/system/include
	EXTRA_LDFLAGS := -L$(SYSROOT)/system/lib -lgdi32
	EXE := windowserver.exe
else
	-include ../../ghost.mk
	LINPUT := -linput
	LVIDEO := -lvideo
	LPS2DRIVER := -lps2driver
	EXTRA_CXXFLAGS :=
	EXTRA_LDFLAGS :=
	EXE := windowserver.bin
endif

CXXFLAGS := -Isrc -I$(FREETYPE_INC) -Wall -Wextra -std=c++17 $(EXTRA_CXXFLAGS)
LDFLAGS := -ljson $(LINPUT) $(LVIDEO) $(LPS2DRIVER) -lproperties -lwindow -lfont -lcairo -lfreetype -lpixman-1 -lpng -lz $(EXTRA_LDFLAGS)
SRC := $(shell find src -name '*.cpp')
OBJ := $(patsubst src/%.cpp,obj/%.o,$(SRC))

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(EXE)
	mkdir -p $(SYSROOT)/applications
	cp $(EXE) $(SYSROOT)/applications/

clean:
	rm -rf obj $(EXE)

.PHONY: all install clean
