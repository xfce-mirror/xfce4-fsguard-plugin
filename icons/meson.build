sizes = [16, 22, 24, 32, 36, 48]
names = [
  'xfce4-fsguard-plugin',
  'xfce4-fsguard-plugin-urgent',
  'xfce4-fsguard-plugin-warning',
]

foreach size : sizes
  foreach name : names
    install_data(
      '@0@x@0@'.format(size) / '@0@.png'.format(name),
      install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
    )
  endforeach
endforeach

foreach name : names
  install_data(
    'scalable' / '@0@.svg'.format(name),
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
  )
endforeach
