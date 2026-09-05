# Tests

m_dep = meson.get_compiler('c').find_library('m', required: false)

test_render_sources = files(
  'test-render.c',
)

test_render_exe = executable('test-render',
  test_render_sources,
  dependencies: [
    glib_dep,
    gobject_dep,
    gdk_pixbuf_dep,
    engine_dep,
    m_dep,
  ],
  c_args: [
    '-I' + join_paths(meson.current_source_dir(), '..', 'engine'),
  ],
)

test('render', test_render_exe)
