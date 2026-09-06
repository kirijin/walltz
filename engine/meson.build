# C engine library — pure C, no Qt/GTK deps

gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: '>= 2.42')
m_dep = meson.get_compiler('c').find_library('m', required: false)

engine_sources = files(
  'engine.c',
  'blur.c',
  'effects.c',
  'pattern.c',
  'mood.c',
  'image_io.c',
  'render_composition.c',
  'portal.c',
)

engine_lib = static_library('walltz-engine',
  engine_sources,
  c_args: [
    '-Wall',
    '-Wextra',
    '-Wno-unused-parameter',
    '-O2',
    '-ffast-math',
  ],
  dependencies: [glib_dep, gobject_dep, gdk_pixbuf_dep, m_dep],
  install: false,
)

engine_dep = declare_dependency(
  link_with: engine_lib,
  include_directories: include_directories('.'),
  dependencies: [glib_dep, gobject_dep, gdk_pixbuf_dep, m_dep, gio_dep],
)
