# Data files (desktop, metainfo, icons, gschema)

gnome = import('gnome')

# Desktop file
install_data('org.walltz.walltz.desktop',
  install_dir: join_paths(get_option('datadir'), 'applications')
)

# Metainfo
install_data('org.walltz.walltz.metainfo.xml',
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

# GSettings schema
install_data('org.walltz.walltz.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas')
)

# Icons
icon_sizes = ['32', '48', '64', '128']
foreach size : icon_sizes
  install_data('icons/@0@x@0@/org.walltz.walltz.svg'.format(size, size),
    install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', '@0@x@0@'.format(size, size), 'apps')
  )
endforeach

# Compile GSettings
gnome.compile_schemas(
  build_by_default: true,
  depend_files: files('org.walltz.walltz.gschema.xml'),
)
