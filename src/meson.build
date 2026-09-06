# Vala application sources

gnome = import('gnome')

# Vala sources
vala_sources = files(
  'application.vala',
  'window.vala',
  'controls.vala',
  'drop-area.vala',
  'settings.vala',
  'vapi/engine.vapi',
)

# GResource
gresource_xml = 'walltz.gresource.xml'
gresource_source = custom_target('gresource',
  input: gresource_xml,
  output: 'walltz.gresource.c',
  command: [
    'glib-compile-resources',
    '--target', '@OUTPUT@',
    '--generate-source',
    '--sourcedir', meson.current_source_dir(),
    '@INPUT@',
  ],
)

# Compile Vala
executable('walltz',
  vala_sources + [gresource_source],
  dependencies: [
    glib_dep,
    gobject_dep,
    gio_dep,
    gtk_dep,
    granite_dep,
    engine_dep,
  ],
  vala_args: [
    '--target-glib=2.70',
    '--pkg', 'gtk4',
    '--pkg', 'granite-7',
    '--pkg', 'gdk-pixbuf-2.0',
    '--vapidir', join_paths(meson.current_source_dir(), 'vapi'),
  ],
  c_args: [
    '-I' + join_paths(meson.current_build_dir(), '..', 'engine'),
  ],
  install: true,
)
