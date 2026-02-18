project('compositor', 'c',
  default_options: ['c_std=c11', 'warning_level=2'])

wlroots = dependency('wlroots-0.18')
wayland_server = dependency('wayland-server')
wayland_protocols = dependency('wayland-protocols')
xkbcommon = dependency('xkbcommon')
math = meson.get_compiler('c').find_library('m', required: true)
json = dependency('json-c')
cairo = dependency('cairo')
pangocairo = dependency('pangocairo')
libdrm = dependency('libdrm')
xwayland = dependency('xwayland')
xcb = dependency('xcb')
xcb_icccm = dependency('xcb-icccm')

wl_protocol_dir = wayland_protocols.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')

# Core protocols
protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
  'protocols/wlr-layer-shell-unstable-v1.xml',
]

# Additional protocols for advanced features
additional_protocols = [
  wl_protocol_dir / 'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml',
  wl_protocol_dir / 'unstable/relative-pointer/relative-pointer-unstable-v1.xml',
  wl_protocol_dir / 'unstable/keyboard-shortcuts-inhibit/keyboard-shortcuts-inhibit-unstable-v1.xml',
]

protocols += additional_protocols

protocol_headers = []
protocol_code = []

foreach p : protocols
  protocol_headers += custom_target(
    p.underscorify() + '_header',
    input: p,
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
  )
  protocol_code += custom_target(
    p.underscorify() + '_code',
    input: p,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
  )
endforeach

src = [
  'src/main.c',
  'src/core.c',
  'src/shell.c',
  'src/config.c',
  'src/toml.c',
  'src/gesture.c',
  'src/visual.c',
  'src/visuals/stub.c',
  'src/visuals/decor.c',
  'src/visuals/themes.c',
  'src/window.c',
  'src/protocols.c',
  'src/background.c',
  'src/anim.c',
  'src/rules.c',
  'src/util.c',
  'src/titlebar.c',
  'src/titlebar_render.c',
  'src/ipc.c',
]

executable('compositor', src, protocol_headers, protocol_code,
  dependencies: [wlroots, wayland_server, xkbcommon, math, json, xwayland, xcb, xcb_icccm, cairo, pangocairo, libdrm],
  include_directories: include_directories('.'),
  c_args: ['-Wno-incompatible-pointer-types'])
