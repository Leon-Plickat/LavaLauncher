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
    'lavalauncher.1.scd'
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
        sh, '-c', '@0@ < @INPUT@ > doc/@1@'.format(scdoc_prog.path(), output)
      ],
      install: true,
      install_dir: '@0@/man@1@'.format(mandir, section)
    )
  endforeach
endif

