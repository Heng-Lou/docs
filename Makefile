#
# Simple Makefile for DOCA Flow example
#

APP_NAME = doca_flow_simple

PKGCONF ?= pkg-config
export PKG_CONFIG_PATH := /opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:$(PKG_CONFIG_PATH)

CFLAGS += -O3 -g
CFLAGS += $(shell $(PKGCONF) --cflags doca-flow libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs doca-flow libdpdk)

SRCS = doca_flow_simple.c
OBJS = $(SRCS:.c=.o)

all: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(APP_NAME) $(OBJS)

.PHONY: all clean
