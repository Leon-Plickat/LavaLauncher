WAYLAND_SCANNER=$(shell pkg-config --variable=wayland_scanner wayland-scanner)

LIBS=\
	 $(shell pkg-config --cflags --libs wayland-server) \
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

lavalauncher: lavalauncher.c lib/wlr-layer-shell-unstable-v1-client-protocol.h lib/wlr-layer-shell-unstable-v1-client-protocol.c
	$(CC) $(CFLAGS) \
		-g \
		-o $@ $< \
		$(LIBS)

clean:
	rm -f lavalauncher lib/wlr-layer-shell-unstable-v1-client-protocol.h lib/wlr-layer-shell-unstable-v1-client-protocol.c

.DEFAULT_GOAL=lavalauncher
.PHONY: clean
