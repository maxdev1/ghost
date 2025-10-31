SUBPROJECTS := libjson libproperties libwindow libfont windowserver

.PHONY: all clean

all: build install

build:
	@$(MAKE) -C libjson
	@$(MAKE) -C libjson install
	@$(MAKE) -C libproperties
	@$(MAKE) -C libproperties install
	@$(MAKE) -C libwindow
	@$(MAKE) -C libwindow install
	@$(MAKE) -C windowserver

install:
	@$(MAKE) -C windowserver install

clean:
	@for dir in $(SUBPROJECTS); do \
		$(MAKE) -C $$dir clean; \
	done
