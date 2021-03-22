wp_dir = wayland_protocols.get_pkgconfig_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
  wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner'),
  native: true,
)

protocols = [
  [ wp_dir, 'stable/xdg-shell/xdg-shell.xml' ],
  [ wp_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml' ],
  [ 'wlr-layer-shell-unstable-v1.xml' ],
  [ 'wlr-foreign-toplevel-management-unstable-v1.xml' ],
  [ 'river-status-unstable-v1.xml' ],
]

wl_protocols_src     = []
wl_protocols_headers = []
foreach p : protocols
  xml = join_paths(p)
  wl_protocols_src += custom_target(
    xml.underscorify() + '_server_c',
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [ wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@' ],
  )
  wl_protocols_headers += custom_target(
    xml.underscorify() + '_client_h',
    input: xml,
    output: '@BASENAME@-protocol.h',
    command: [ wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@' ],
  )
endforeach

wl_protocols_lib = static_library(
  'lavalauncher_protocols',
  wl_protocols_src + wl_protocols_headers,
  dependencies: [ wayland_client ],
)

wl_protocols = declare_dependency(
  link_with: wl_protocols_lib,
  sources: wl_protocols_headers,
)
