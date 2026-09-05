project('walltz', 'c', 'vala',
  version: '0.1.0',
  meson_version: '>= 0.59.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'buildtype=debugoptimized',
  ]
)

# Dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.70')
gobject_dep = dependency('gobject-2.0', version: '>= 2.70')
gio_dep = dependency('gio-2.0', version: '>= 2.70')
gtk_dep = dependency('gtk4', version: '>= 4.6')
granite_dep = dependency('granite-7', version: '>= 7.0')

# C engine library
subdir('engine')

# Vala application
subdir('src')

# Data (icons, desktop, metainfo)
subdir('data')

# Tests
subdir('tests')
