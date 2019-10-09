PREFIX = /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

WAYLAND_PROTOCOLS=$(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

LIBS=\
	 $(shell pkg-config --cflags --libs wayland-server) \
	 $(shell pkg-config --cflags --libs wayland-client) \
	 $(shell pkg-config --cflags --libs cairo)

CFLAGS=\
       -std=c11 \
       -Werror \
       -Wall \
       -Wextra \
       -Wpedantic \
       -Wno-unused-variable \
       -Wno-unused-function \
       -Wno-unused-parameter \
       -Ilib

lib/wlr-layer-shell-unstable-v1-client-protocol.h: lib/wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) server-header lib/wlr-layer-shell-unstable-v1.xml $@

lib/wlr-layer-shell-unstable-v1-client-protocol.c: lib/wlr-layer-shell-unstable-v1.xml
	$(WAYLAND_SCANNER) private-code lib/wlr-layer-shell-unstable-v1.xml $@

lib/xdg-output-unstable-v1-client-protocol.h:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@

lib/xdg-output-unstable-v1-client-protocol.c:
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@

lavalauncher:\
	lavalauncher.c \
	lib/wlr-layer-shell-unstable-v1-client-protocol.h \
	lib/wlr-layer-shell-unstable-v1-client-protocol.c \
	lib/xdg-output-unstable-v1-client-protocol.h \
	lib/xdg-output-unstable-v1-client-protocol.c
	$(CC) $(CFLAGS) \
		-g \
		-o $@ $< \
		$(LIBS)

install: lavalauncher lavalauncher.1
	install -D -m 755 lavalauncher $(BINPREFIX)/lavalauncher
	install -D -m 644 lavalauncher.1 $(MANPREFIX)/man1/lavalauncher.1

uninstall:
	$(RM) $(BINPREFIX)/lavalauncher
	$(RM) $(MANPREFIX)/man1/lavalauncher.1

clean:
	rm -f lavalauncher \
		lib/wlr-layer-shell-unstable-v1-client-protocol.h \
		lib/wlr-layer-shell-unstable-v1-client-protocol.c \
		lib/xdg-output-unstable-v1-client-protocol.h \
		lib/xdg-output-unstable-v1-client-protocol.c

.DEFAULT_GOAL=lavalauncher
.PHONY: clean
