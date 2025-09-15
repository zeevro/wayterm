# CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -g
CFLAGS ?= -std=c17
PKG_CONFIG ?= pkg-config

# Host deps
WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-server --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)

# Build deps
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

HEADERS=xdg-shell-server-protocol.h
SOURCES=main.c xdg-shell-protocol.c

all: wayterm

wayterm: $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(WAYLAND_FLAGS)

xdg-shell-server-protocol.h:
	$(WAYLAND_SCANNER) server-header $(XDG_SHELL_PROTOCOL) $@

xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $@

.PHONY: clean
clean:
	$(RM) wayterm xdg-shell-protocol.c xdg-shell-server-protocol.h
