# Building {#building-xkbcommon}

xkbcommon requires:

- a C compiler supporting C11
- XKB registry (optional): `libxml2`
- X11 features (optional): `libxcb` and `libxcb-xkb`
- Wayland features (optional): `wayland-client`, `wayland-protocols`, `wayland-scanner`

xkbcommon is built with [Meson](http://mesonbuild.com):

```shell
meson setup build
meson compile -C build
meson test -C build # Run the tests.
```

To build for use with Wayland, you can disable X11 support while still
using the X11 keyboard configuration resource files thusly:

```shell
meson setup build \
      -Denable-x11=false \
      -Dxkb-config-root=/usr/share/X11/xkb \
      -Dx-locale-root=/usr/share/X11/locale
meson compile -C build
```

<details>
<summary>Complete list of user options</summary>
@include ../meson.options
</details>
