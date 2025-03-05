sizes = [16, 24, 32, 48, 64, 96, 128]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'org.xfce.taskmanager.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  'scalable' / 'org.xfce.taskmanager.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

install_data(
  'scalable' / 'xc_crosshair-symbolic.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'actions',
)
