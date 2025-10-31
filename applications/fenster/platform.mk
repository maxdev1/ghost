ifeq ($(target),ghost)
	SYSROOT := /ghost/source/sysroot
	CXX := /ghost/bin/x86_64-ghost-g++
	AR := /ghost/bin/x86_64-ghost-ar

	FREETYPE_INC := $(SYSROOT)/system/include/freetype2

else ifeq ($(target),windows)
	SYSROOT := ../platform/windows-mingw/sysroot
	CXX := x86_64-w64-mingw32-g++
	AR := x86_64-w64-mingw32-ar

	FREETYPE_INC := /usr/x86_64-w64-mingw32/include/freetype2
	CAIRO_INC := /usr/x86_64-w64-mingw32/include

else ifeq ($(target),)
$(error Target must be specified (e.g. "make target=ghost"))
endif