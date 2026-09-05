# Tests

m_dep = meson.get_compiler('c').find_library('m', required: false)

# Render test
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

# Mood test
test_mood_sources = files(
  'test-mood.c',
)

test_mood_exe = executable('test-mood',
  test_mood_sources,
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

test('mood', test_mood_exe)

# Pattern test
test_pattern_sources = files(
  'test-pattern.c',
)

test_pattern_exe = executable('test-pattern',
  test_pattern_sources,
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

test('pattern', test_pattern_exe)
