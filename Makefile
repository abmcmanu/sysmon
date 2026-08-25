# sysmon
#
#   make             build sysmond
#                    + sysmonctl
#                    + plugins
#   make test        unit tests
#   make smoke       start a daemon and poke at it
#   make asan        build and test with ASan + UBSan (in build-asan/)
#   make install     PREFIX=/usr/local DESTDIR=

CC     ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Isrc
CFLAGS += -Iinclude
CFLAGS += -MMD -MP
LDLIBS  =
BUILD  ?= build
PREFIX ?= /usr/local

ifeq ($(SANITIZE),1)
CFLAGS  += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address,undefined
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
PLATFORM_SRC = src/collect_darwin.c
else ifeq ($(UNAME_S),FreeBSD)
PLATFORM_SRC = src/collect_freebsd.c
else
PLATFORM_SRC = src/collect_linux.c
CFLAGS += -D_GNU_SOURCE
LDLIBS += -ldl
endif

LIB_SRC = src/config.c src/log.c src/metrics.c $(PLATFORM_SRC)
LIB_SRC += src/collect_disk.c
LIB_SRC += src/ring.c
LIB_SRC += src/sb.c
LIB_SRC += src/server.c
LIB_SRC += src/plugins.c
LIB_SRC += src/http.c
LIB_SRC += src/json.c
LIB_SRC += src/alert.c

LIB_OBJ  = $(LIB_SRC:%.c=$(BUILD)/obj/%.o)
MAIN_OBJ = $(BUILD)/obj/src/main.o

CTL_SRC = tools/sysmonctl.c src/sb.c
CTL_SRC += tools/client.c
CTL_SRC += tools/tui.c
CTL_OBJ = $(CTL_SRC:%.c=$(BUILD)/obj/%.o)

PLUGINS =
PLUGINS += $(BUILD)/plugins/uptime.so
PLUGINS += $(BUILD)/plugins/net.so
PLUGINS += $(BUILD)/plugins/cpustat.so
PLUGINS += $(BUILD)/plugins/procs.so
PLUGINS += $(BUILD)/plugins/fd.so
PLUGINS += $(BUILD)/plugins/diskio.so
PLUGINS += $(BUILD)/plugins/psi.so
PLUGINS += $(BUILD)/plugins/thermal.so

TESTS = $(BUILD)/tests/test_ring
TESTS += $(BUILD)/tests/test_config
TESTS += $(BUILD)/tests/test_sb
TESTS += $(BUILD)/tests/test_alert
TESTS += $(BUILD)/tests/test_server
TESTS += $(BUILD)/tests/test_json
TESTS += $(BUILD)/tests/test_metrics
TESTS += $(BUILD)/tests/test_disk
TESTS += $(BUILD)/tests/test_http
TESTS += $(BUILD)/tests/test_plugins
TESTS += $(BUILD)/tests/test_log

all: $(BUILD)/sysmond $(BUILD)/sysmonctl plugins

$(BUILD)/sysmond: $(MAIN_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/sysmonctl: $(CTL_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

plugins: $(PLUGINS)

# Plugins only see include/sysmon/plugin.h, never the daemon's internals.
$(BUILD)/plugins/%.so: plugins/%.c include/sysmon/plugin.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LDFLAGS)

CFLAGS += -I$(BUILD)/gen

$(BUILD)/gen/web_index.h: web/index.html scripts/embed.sh
	@mkdir -p $(@D)
	sh scripts/embed.sh web_index web/index.html > $@

$(BUILD)/obj/src/http.o: $(BUILD)/gen/web_index.h

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests/%: tests/%.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -Itests -o $@ $^ $(LDLIBS)

TEST_PLUGINS = $(BUILD)/tests/plugin_test.so
TEST_PLUGINS += $(BUILD)/tests/plugin_bad.so

$(BUILD)/tests/%.so: tests/%.c include/sysmon/plugin.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LDFLAGS)

test: $(TESTS) $(TEST_PLUGINS)
	@for t in $(TESTS); do $$t $(BUILD)/tests || exit 1; done

smoke: all
	BUILD=$(BUILD) sh tests/smoke.sh

FUZZ_TARGETS = $(BUILD)-fuzz/fuzz/fuzz_config
FUZZ_TARGETS += $(BUILD)-fuzz/fuzz/fuzz_command
FUZZ_TARGETS += $(BUILD)-fuzz/fuzz/fuzz_alert
FUZZ_CFLAGS = -g -O1 -std=c11 -fsanitize=fuzzer,address,undefined -Isrc -Iinclude -I$(BUILD)/gen

# needs clang with libFuzzer; see fuzz/run.sh
fuzz: $(FUZZ_TARGETS)

$(BUILD)-fuzz/fuzz/%: fuzz/%.c $(LIB_SRC) $(BUILD)/gen/web_index.h
	@mkdir -p $(@D)
	clang $(FUZZ_CFLAGS) $(filter -D%,$(CFLAGS)) -o $@ $< $(LIB_SRC) $(LDLIBS)

$(BUILD)/bench/bench: bench/bench.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

bench: $(BUILD)/bench/bench
	$(BUILD)/bench/bench

asan:
	$(MAKE) BUILD=build-asan SANITIZE=1 all test

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib/sysmon/plugins
	install -m 755 $(BUILD)/sysmond $(BUILD)/sysmonctl $(DESTDIR)$(PREFIX)/bin/
	install -m 644 $(PLUGINS) $(DESTDIR)$(PREFIX)/lib/sysmon/plugins/
	install -d $(DESTDIR)$(PREFIX)/share/man/man8
	install -m 644 docs/sysmond.8 $(DESTDIR)$(PREFIX)/share/man/man8/
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 docs/sysmonctl.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	install -d $(DESTDIR)$(PREFIX)/share/bash-completion/completions \
	    $(DESTDIR)$(PREFIX)/share/zsh/site-functions \
	    $(DESTDIR)$(PREFIX)/share/fish/vendor_completions.d
	install -m 644 contrib/completions/sysmonctl.bash \
	    $(DESTDIR)$(PREFIX)/share/bash-completion/completions/sysmonctl
	install -m 644 contrib/completions/_sysmonctl $(DESTDIR)$(PREFIX)/share/zsh/site-functions/
	install -m 644 contrib/completions/sysmonctl.fish \
	    $(DESTDIR)$(PREFIX)/share/fish/vendor_completions.d/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sysmond $(DESTDIR)$(PREFIX)/bin/sysmonctl
	rm -rf $(DESTDIR)$(PREFIX)/lib/sysmon
	rm -f $(DESTDIR)$(PREFIX)/share/man/man8/sysmond.8
	rm -f $(DESTDIR)$(PREFIX)/share/bash-completion/completions/sysmonctl \
	    $(DESTDIR)$(PREFIX)/share/zsh/site-functions/_sysmonctl \
	    $(DESTDIR)$(PREFIX)/share/fish/vendor_completions.d/sysmonctl.fish

VERSION := $(shell sed -n 's/^#define SYSMON_VERSION "\(.*\)"/\1/p' src/version.h)

# source tarball of HEAD, what the release workflow uploads
dist:
	git archive --prefix=sysmon-$(VERSION)/ -o sysmon-$(VERSION).tar.gz HEAD
	@echo sysmon-$(VERSION).tar.gz

check: test smoke

format:
	clang-format -i src/*.[ch] tools/*.[ch] plugins/*.c tests/*.[ch] include/sysmon/*.h

clean:
	rm -rf $(BUILD) build-asan build-fuzz

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all plugins test smoke asan install uninstall clean
