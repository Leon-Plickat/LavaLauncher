project(
  'lavalauncher',
  'c',
  version: '2.0.0', # release version
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
  '-Wno-pointer-arith',
  '-Wno-unused-parameter',
  '-Wformat-security',
  '-Wunreachable-code',
  '-Wformat',
  '-Wconversion',
  '-Wsign-conversion',
  '-Wfloat-conversion',
  '-Wunused-result',
]), language: 'c')

if get_option('watch-config').enabled()
  add_project_arguments(cc.get_supported_arguments([ '-DWATCH_CONFIG' ]), language: 'c')
endif

version = '"@0@"'.format(meson.project_version())
git = find_program('git', native: true, required: false)
if git.found()
  git_commit = run_command([git, 'rev-parse', '--short', 'HEAD'])
  git_branch = run_command([git, 'rev-parse', '--abbrev-ref', 'HEAD'])
  if git_commit.returncode() == 0 and git_branch.returncode() == 0
    version = '"GIT-@0@ (" __DATE__ ", branch \'@1@\')"'.format(
      git_commit.stdout().strip(),
      git_branch.stdout().strip(),
    )
  endif
endif
add_project_arguments('-DLAVALAUNCHER_VERSION=@0@'.format(version), language: 'c')

wayland_protocols = dependency('wayland-protocols')
wayland_client    = dependency('wayland-client', include_type: 'system')
wayland_cursor    = dependency('wayland-cursor', include_type: 'system')
cairo             = dependency('cairo')
realtime          = cc.find_library('rt')
librsvg           = dependency('librsvg-2.0', version: '>= 2.45.6', required: get_option('librsvg'))
xkbcommon         = dependency('xkbcommon')

if ['dragonfly', 'freebsd', 'netbsd', 'openbsd'].contains(host_machine.system())
  libinotify = dependency('libinotify', required: get_option('watch-config'))
  add_project_arguments(cc.get_supported_arguments([ '-DBSD' ]), language: 'c')
else
  libinotify = []
endif

if librsvg.found()
  add_project_arguments(cc.get_supported_arguments([ '-DSVG_SUPPORT' ]), language: 'c')
endif

subdir('protocol')
subdir('doc')

summary({
  'watch-config': get_option('watch-config').enabled(),
  'man-pages': get_option('man-pages').enabled(),
  'librsvg': librsvg.found(),
}, bool_yn: true)

executable(
  'lavalauncher',
  files(
    'lib/lib-infinitesimal.c',
    'src/bar.c',
    'src/config-parser.c',
    'src/event-loop.c',
    'src/foreign-toplevel-management.c',
    'src/inotify-event-source.c',
    'src/item.c',
    'src/lavalauncher.c',
    'src/output.c',
    'src/seat.c',
    'src/signal-event-source.c',
    'src/types/box_t.c',
    'src/types/buffer.c',
    'src/types/colour_t.c',
    'src/types/image_t.c',
    'src/util.c',
    'src/wayland-connection.c',
  ),
  dependencies: [
    cairo,
    libinotify,
    librsvg,
    realtime,
    wayland_client,
    wayland_cursor,
    wayland_protocols,
    wl_protocols,
    xkbcommon,
  ],
  include_directories: include_directories('src', 'lib'),
  install: true,
)

