# -*- makefile -*-

include Make.vars

DIRS = $(sort $(addprefix build/,$(KERNEL_SUBDIRS)))

all: $(DIRS) build/Makefile
	cd build && $(MAKE) $@
	
$(DIRS):
	mkdir -p $@
	
build/Makefile: ./Makefile.build
	cp $< $@

build/%: $(DIRS) build/Makefile
	cd build && $(MAKE) $*

clean:
	rm -rf build

