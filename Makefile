# sysmon
#
#   make             build sysmond
#                    + sysmonctl

CC     ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Isrc
LDLIBS  =
BUILD  ?= build

PLATFORM_SRC = src/collect_linux.c
CFLAGS += -D_GNU_SOURCE

LIB_SRC = src/config.c src/log.c src/metrics.c $(PLATFORM_SRC)
LIB_SRC += src/collect_disk.c
LIB_SRC += src/ring.c
LIB_SRC += src/sb.c
LIB_SRC += src/server.c

LIB_OBJ  = $(LIB_SRC:%.c=$(BUILD)/obj/%.o)
MAIN_OBJ = $(BUILD)/obj/src/main.o

CTL_SRC = tools/sysmonctl.c src/sb.c
CTL_OBJ = $(CTL_SRC:%.c=$(BUILD)/obj/%.o)

all: $(BUILD)/sysmond $(BUILD)/sysmonctl

$(BUILD)/sysmond: $(MAIN_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/sysmonctl: $(CTL_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD)

.PHONY: all clean
