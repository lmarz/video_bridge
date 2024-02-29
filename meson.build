project('video_bridge',
        'c',
        version : '0.1', default_options: ['warning_level=3'])

cc = meson.get_compiler('c')
if cc.has_argument('-Wno-pedantic')
    add_global_arguments('-Wno-pedantic', language : 'c')
endif

dbus = dependency('dbus-1')
pipewire = dependency('libpipewire-0.3')
xlib = dependency('x11')

deps = [dbus, pipewire, xlib]

sources = files([
    'main.c',
    'dbus_handler.c',
    'pipewire_handler.c',
    'x11_handler.c'
])

executable('video_bridge', sources, dependencies: deps, install : true)
