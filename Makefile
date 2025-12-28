# NetPulse Makefile
# Works on macOS and Linux

CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -g
LDFLAGS =

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -DPLATFORM_MACOS
    # macOS: use stub ICMP implementation
    ICMP_SRC = src/net/icmp_probe_stub.c
else
    CFLAGS += -DPLATFORM_LINUX -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
    CFLAGS += -DHAS_ICMP_PROBE
    LDFLAGS += -lpthread
    # Linux: use real ICMP implementation
    ICMP_SRC = src/net/icmp_probe_linux.c
endif

# Mongoose configuration
CFLAGS += -DMG_ENABLE_LINES=1 -DMG_ENABLE_DIRECTORY_LISTING=0

# Include paths
INCLUDES = -Isrc -Ithird_party/mongoose

# Source files
SRCS = src/main.c \
       src/platform/time.c \
       src/platform/fs.c \
       src/core/ring_buffer.c \
       src/core/config.c \
       src/core/stats.c \
       src/core/event_log.c \
       src/core/scheduler.c \
       src/net/dns.c \
       src/net/tcp_probe.c \
       $(ICMP_SRC) \
       src/server/server.c \
       src/server/http_handlers.c \
       src/server/ws_handlers.c \
       third_party/mongoose/mongoose.c

# Object files
OBJDIR = build/obj
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

# Output
TARGET = build/netpulsed

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build

# Run the daemon
run: $(TARGET)
	./$(TARGET)
