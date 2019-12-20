PREFIX    = /usr/local
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

WAYLAND_PROTOCOLS = $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER   = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

LIBS=\
	 $(shell pkg-config --libs wayland-client) \
	 $(shell pkg-config --libs wayland-protocols) \
	 $(shell pkg-config --libs cairo) \
	 -lrt

CFLAGS=\
       -std=c11 \
       -O2 \
       -Werror \
       -Wall \
       -Wextra \
       -Wpedantic \
       -Wno-maybe-uninitialized \
       -Wno-unused-variable \
       -Wno-unused-function \
       -Wno-unused-parameter \
       -Wno-implicit-fallthrough \
       -Ilib/wayland-protocols \
       -Ilib/pool-buffer \
       $(shell pkg-config --cflags wayland-client) \
       $(shell pkg-config --cflags wayland-protocols) \
       $(shell pkg-config --cflags cairo)

lib/wayland-protocols/wlr-layer-shell-unstable-v1-client-protocol.h: lib/wayland-protocols/wlr-layer-shell-unstable-v1.xml
	@echo Generating $@
	$(WAYLAND_SCANNER) client-header $< $@

lib/wayland-protocols/wlr-layer-shell-unstable-v1-protocol.c: lib/wayland-protocols/wlr-layer-shell-unstable-v1.xml
	@echo Generating $@
	$(WAYLAND_SCANNER) private-code $< $@

lib/wayland-protocols/xdg-shell-client-protocol.h:
	@echo Generating $@
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

lib/wayland-protocols/xdg-shell-protocol.c:
	@echo Generating $@
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

lib/wayland-protocols/xdg-output-unstable-v1-client-protocol.h:
	@echo Generating $@
	$(WAYLAND_SCANNER) client-header $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@

lib/wayland-protocols/xdg-output-unstable-v1-protocol.c:
	@echo Generating $@
	$(WAYLAND_SCANNER) private-code $(WAYLAND_PROTOCOLS)/unstable/xdg-output/xdg-output-unstable-v1.xml $@

pool-buffer.o: lib/pool-buffer/pool-buffer.c lib/pool-buffer/pool-buffer.h
	@echo Building $@
	$(CC) $(CFLAGS) -c $<

config.o: src/config.c src/config.h \
	lib/wayland-protocols/wlr-layer-shell-unstable-v1-client-protocol.h
	@echo Building $@
	$(CC) $(CFLAGS) -c $<

draw.o: src/draw.c src/draw.h
	@echo Building $@
	$(CC) $(CFLAGS) -c $<

lavalauncher.o: src/lavalauncher.c src/lavalauncher.h \
	lib/wayland-protocols/wlr-layer-shell-unstable-v1-client-protocol.h \
	lib/wayland-protocols/xdg-shell-client-protocol.h \
	lib/wayland-protocols/xdg-output-unstable-v1-client-protocol.h \
	lib/pool-buffer/pool-buffer.h
	@echo Building $@
	$(CC) $(CFLAGS) -c $<

lavalauncher: lavalauncher.o pool-buffer.o config.o draw.o \
	lib/wayland-protocols/wlr-layer-shell-unstable-v1-protocol.c \
	lib/wayland-protocols/xdg-output-unstable-v1-protocol.c \
	lib/wayland-protocols/xdg-shell-protocol.c
	@echo Building $@
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

doc/lavalauncher.1: doc/lavalauncher.1.scd
	@echo Generating $@
	scdoc < $^ > $@

install: lavalauncher doc/lavalauncher.1
	@echo Installing
	install -D -m 755 lavalauncher $(BINPREFIX)/lavalauncher
	install -D -m 644 doc/lavalauncher.1 $(MANPREFIX)/man1/lavalauncher.1

uninstall:
	@echo Uninstalling
	$(RM) $(BINPREFIX)/lavalauncher
	$(RM) $(MANPREFIX)/man1/lavalauncher.1

clean:
	@echo Cleaning
	rm -f lavalauncher \
		lib/wayland-protocols/wlr-layer-shell-unstable-v1-client-protocol.h \
		lib/wayland-protocols/wlr-layer-shell-unstable-v1-protocol.c \
		lib/wayland-protocols/xdg-shell-client-protocol.h \
		lib/wayland-protocols/xdg-shell-protocol.c \
		lib/wayland-protocols/xdg-output-unstable-v1-client-protocol.h \
		lib/wayland-protocols/xdg-output-unstable-v1-protocol.c \
		pool-buffer.o \
		lavalauncher.o \
		config.o \
		draw.o \
		doc/lavalauncher.1

.DEFAULT_GOAL=lavalauncher
.PHONY: clean
