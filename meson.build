project('ambiente', 'cpp',
  version: '0.1',
  default_options: ['cpp_std=c++17', 'warning_level=3']
)

fs = import('python')

src = include_directories('app')

cpp_files = run_command('find', 'app', '-name', '*.cpp').stdout().strip().split('\n')

srcs = files('ambiente.cpp')
foreach f : cpp_files
  srcs += files(f)
endforeach

xcb = dependency('xcb', required: true)
xcb_ewmh = dependency('xcb-ewmh', required: true)
xcb_keysyms = dependency('xcb-keysyms', required: true)
xcb_shape = dependency('xcb-shape', required: true)
xcb_damage = dependency('xcb-damage', required: true)
xcb_xfixes = dependency('xcb-xfixes', required: true)
xcb_composite = dependency('xcb-composite', required: true)
xcb_render = dependency('xcb-render', required: true)
xcb_render_util = dependency('xcb-renderutil', required: true)
cairo = dependency('cairo', required: true)

executable('ambiente', srcs,
  include_directories: src,
  dependencies: [
    xcb, xcb_ewmh, xcb_keysyms,
    xcb_shape, xcb_damage, xcb_xfixes,
    xcb_composite, xcb_render, xcb_render_util,
    cairo
  ],
  install: false
)
