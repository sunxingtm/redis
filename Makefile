# Top level makefile, the real shit is at src/Makefile

TARGETS=32bit noopt test

all:
	cd src && $(MAKE) $@

install: dummy
	cd src && $(MAKE) $@

clean:
	cd src && $(MAKE) $@
	cd deps/hiredis && $(MAKE) $@
	cd deps/linenoise && $(MAKE) $@
	-(cd deps/jemalloc && $(MAKE) distclean)
	cd setup && $(MAKE) $@

$(TARGETS):
	cd src && $(MAKE) $@

setup: all
	cd setup && $(MAKE) $@

src/help.h:
	@./utils/generate-command-help.rb > $@

dummy:
