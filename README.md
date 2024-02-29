# Video Bridge
Allows screensharing of wayland windows/screens to x11 apps running over
xwayland.

This is basically exactly like
[xwaylandvideobridge](https://invent.kde.org/system/xwaylandvideobridge/), but
with worse functionality (for now) and a smaller file size.

This program simply uses the low level apis like dbus and xlib directly without
using a high level toolkit like QT or GTK, because they're too gigantic for this
simple functionality.

## How to build
### Dependencies
You need the following dependencies:
- libdbus
- libpipewire
- libx11

### Building & Installing
```sh
$ meson setup <builddir>
$ cd <builddir>
$ sudo ninja install
```

## How to Use
Simply execute
```sh
$ video_bridge
```
and select the wayland window or screen you want to share. Now you can go to your x11 app and will be able to select your application/screen (currently named "Video Bridge").

## License
This project is licensed under the terms of the European Union Public Licence v1.2 (EUPL-1.2). See [LICENSE](LICENSE) for more details.