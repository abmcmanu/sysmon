# sysmon - shell edition
#
#   make test      run the bats test suite
#   make install   install as $(PREFIX)/bin/sysmon

PREFIX ?= /usr/local
SCRIPT  = legacy/sysmon.sh
TESTDIR = legacy/tests

all:
	@echo "nothing to build, try: make test | make install"

test:
	bats $(TESTDIR)

lint:
	shellcheck $(SCRIPT)

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(SCRIPT) $(DESTDIR)$(PREFIX)/bin/sysmon

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sysmon

.PHONY: all test lint install uninstall
