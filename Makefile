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

TESTS = $(BUILD)/tests/test_ring
TESTS += $(BUILD)/tests/test_config
TESTS += $(BUILD)/tests/test_sb
TESTS += $(BUILD)/tests/test_alert
TESTS += $(BUILD)/tests/test_server
TESTS += $(BUILD)/tests/test_json
TESTS += $(BUILD)/tests/test_metrics
TESTS += $(BUILD)/tests/test_disk

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

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests/%: tests/%.c $(LIB_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -Itests -o $@ $^ $(LDLIBS)

test: $(TESTS)
	@for t in $(TESTS); do $$t || exit 1; done

smoke: all
	BUILD=$(BUILD) sh tests/smoke.sh

asan:
	$(MAKE) BUILD=build-asan SANITIZE=1 all test

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/lib/sysmon/plugins
	install -m 755 $(BUILD)/sysmond $(BUILD)/sysmonctl $(DESTDIR)$(PREFIX)/bin/
	install -m 644 $(PLUGINS) $(DESTDIR)$(PREFIX)/lib/sysmon/plugins/
	install -d $(DESTDIR)$(PREFIX)/share/man/man8
	install -m 644 docs/sysmond.8 $(DESTDIR)$(PREFIX)/share/man/man8/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sysmond $(DESTDIR)$(PREFIX)/bin/sysmonctl
	rm -rf $(DESTDIR)$(PREFIX)/lib/sysmon
	rm -f $(DESTDIR)$(PREFIX)/share/man/man8/sysmond.8

clean:
	rm -rf $(BUILD) build-asan

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all plugins test smoke asan install uninstall clean
