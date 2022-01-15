# sysmon
#
#   make             build sysmond
#                    + sysmonctl
#                    + plugins
#   make test        unit tests
#   make smoke       start a daemon and poke at it
#   make asan        build and test with ASan + UBSan (in build-asan/)

CC     ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Isrc
CFLAGS += -Iinclude
CFLAGS += -MMD -MP
LDLIBS  =
BUILD  ?= build

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

LIB_OBJ  = $(LIB_SRC:%.c=$(BUILD)/obj/%.o)
MAIN_OBJ = $(BUILD)/obj/src/main.o

CTL_SRC = tools/sysmonctl.c src/sb.c
CTL_OBJ = $(CTL_SRC:%.c=$(BUILD)/obj/%.o)

PLUGINS =
PLUGINS += $(BUILD)/plugins/uptime.so
PLUGINS += $(BUILD)/plugins/net.so

TESTS = $(BUILD)/tests/test_ring
TESTS += $(BUILD)/tests/test_config
TESTS += $(BUILD)/tests/test_sb

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

clean:
	rm -rf $(BUILD) build-asan

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

.PHONY: all plugins test smoke asan clean
