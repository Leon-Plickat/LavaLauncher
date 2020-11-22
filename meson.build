project(
  'lavalauncher',
  'c',
  version: '1.7.0',
  license: 'GPLv3',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ]
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
  '-fsigned-char',
  '-Wno-unused-parameter',
  '-Wpointer-arith',
  '-Wformat-security',
  '-Wunreachable-code',
  '-Wformat',
  '-Wconversion',
  '-Wfloat-conversion',
]), language: 'c')

if get_option('watch-config').enabled()
  add_project_arguments(cc.get_supported_arguments([ '-DWATCH_CONFIG' ]), language: 'c')
endif

if get_option('handle-signals').enabled()
  add_project_arguments(cc.get_supported_arguments([ '-DHANDLE_SIGNALS' ]), language: 'c')
endif

wayland_protocols = dependency('wayland-protocols')
wayland_client    = dependency('wayland-client', include_type: 'system')
wayland_cursor    = dependency('wayland-cursor', include_type: 'system')
cairo             = dependency('cairo')
realtime          = cc.find_library('rt')
librsvg           = dependency('librsvg-2.0', version: '>= 2.45.6', required: get_option('librsvg'))

if ['dragonfly', 'freebsd', 'netbsd', 'openbsd'].contains(host_machine.system())
  libinotify      = dependency('libinotify', required: get_option('watch-config'))
  libepoll        = dependency('epoll-shim', required: get_option('handle-signals'))
else
  libinotify      = []
  libepoll        = []
endif

if librsvg.found()
  add_project_arguments(cc.get_supported_arguments([ '-DSVG_SUPPORT' ]), language: 'c')
endif

subdir('protocol')

summary({
  'watch-config': get_option('watch-config').enabled(),
  'handle-signals': get_option('handle-signals').enabled(),
  'man-pages': get_option('man-pages').enabled(),
  'librsvg': librsvg.found(),
}, bool_yn: true)

executable(
  'lavalauncher',
  files(
    'src/bar/bar.c',
    'src/bar/bar-pattern.c',
    'src/bar/draw-generics.c',
    'src/bar/indicator.c',
    'src/config.c',
    'src/cursor.c',
    'src/input.c',
    'src/item/button.c',
    'src/item/command.c',
    'src/item/item.c',
    'src/item/spacer.c',
    'src/lavalauncher.c',
    'src/output.c',
    'src/registry.c',
    'src/seat.c',
    'src/str.c',
    'src/types/buffer.c',
    'src/types/colour.c',
    'src/types/image.c',
    'src/types/string_t.c',
  ),
  dependencies: [
    cairo,
    libepoll,
    libinotify,
    librsvg,
    realtime,
    wayland_client,
    wayland_cursor,
    wayland_protocols,
    wl_protocols,
  ],
  include_directories: include_directories('src'),
  install: true,
)

scdoc = dependency(
  'scdoc',
  version: '>=1.9.2',
  native: true,
  required: get_option('man-pages'),
)
if scdoc.found()
  scdoc_prog = find_program(scdoc.get_pkgconfig_variable('scdoc'), native: true)
  sh = find_program('sh', native: true)
  mandir = get_option('mandir')
  man_files = [
    'doc/lavalauncher.1.scd'
  ]
  foreach filename : man_files
    topic = filename.split('.')[-3].split('/')[-1]
    section = filename.split('.')[-2]
    output = '@0@.@1@'.format(topic, section)

    custom_target(
      output,
      input: filename,
      output: output,
      command: [
        sh, '-c', '@0@ < @INPUT@ > @1@'.format(scdoc_prog.path(), output)
      ],
      install: true,
      install_dir: '@0@/man@1@'.format(mandir, section)
    )
  endforeach
endif
