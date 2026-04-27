# Copyright (c) 2026 innodisk Crop.
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT

# --- Variables ---
# Use standard names (TARGET instead of TARGRT)
TARGET     := usb-test-mode
PKG_CONFIG := $(OECORE_NATIVE_SYSROOT)/usr/bin/pkg-config

GIT_HASH := $(shell git rev-parse --short HEAD)
GIT_DIRTY := $(shell git diff --quiet || echo "-dirty")

# Dynamically find source and object files
SRCS       := $(TARGET).c
OBJS       := $(SRCS:.c=.o)

# Flags
# Use := for simple expansion to improve performance
CFLAGS     += -O2 $(shell $(PKG_CONFIG) --cflags libusb-1.0) -DGIT_REVISION=\"$(GIT_HASH)$(GIT_DIRTY)\"
LDFLAGS    += -Wl,--hash-style=both $(shell $(PKG_CONFIG) --libs libusb-1.0)

# --- Rules ---

# The first target is the default when running 'make'
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Standard clean
clean:
	rm -f $(OBJS) $(TARGET)

# --- Special Targets ---
.PHONY: all clean