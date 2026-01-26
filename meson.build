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

wl_protocol_dir = wayland_protocols.get_variable('pkgdatadir')
wayland_scanner = find_program('wayland-scanner')

protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
  'protocols/wlr-layer-shell-unstable-v1.xml',
]

protocol_headers = []
foreach p : protocols
  protocol_headers += custom_target(
    p.underscorify() + '_header',
    input: p,
    output: '@BASENAME@-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
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
  'src/background.c',
  'src/anim.c',
  'src/rules.c',
  'src/util.c',
  'src/titlebar.c',
  'src/titlebar_render.c',
  'src/ipc.c',
]

executable('compositor', src, protocol_headers,
  dependencies: [wlroots, wayland_server, xkbcommon, math, json, xwayland, cairo, pangocairo, libdrm],
  include_directories: include_directories('.'),
  c_args: ['-Wno-incompatible-pointer-types'])
