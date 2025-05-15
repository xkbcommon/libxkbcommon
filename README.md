# libxkbcommon

<!--
NOTE: This file is carefully formatted to support both Github and Doxygen.
They handle line breaks differently!
-->

**libxkbcommon** is a keyboard keymap compiler and support library which
processes a reduced subset of keymaps as defined by the [XKB] \(X Keyboard
Extension) specification. It also contains a module for handling *Compose*
and dead keys, a separate *registry* library for listing available keyboard
layouts and a fair set of <!--!
@rawHtml --><abbr title="Command-Line Interface">CLI</abbr><!--!
@endRawHtml --> *tools*.

[XKB]: doc/introduction-to-xkb.md

## Quick Guide

- [Introduction to XKB][XKB]: to learn the essentials of XKB.
- [User-configuration](doc/user-configuration.md): instructions to add
  a *custom layout* or option.
- [Quick Guide](doc/quick-guide.md): introduction on how to use this library.
- [Tools](./README.md#tools): introduction to the `xkbcli` application.
- [Frequently Asked Question (FAQ)](doc/faq.md).

## Building

libxkbcommon is built with [Meson](http://mesonbuild.com):

```bash
meson setup build
meson compile -C build
meson test -C build # Run the tests.
```

To build for use with Wayland, you can disable X11 support while still
using the X11 keyboard configuration resource files thusly:

```bash
meson setup build \
      -Denable-x11=false \
      -Dxkb-config-root=/usr/share/X11/xkb \
      -Dx-locale-root=/usr/share/X11/locale
meson compile -C build
```

<details>
<summary>Complete list of user options</summary>
@include meson_options.txt
</details>

## API

While libxkbcommon’s API is somewhat derived from the classic XKB API as found
in `X11/extensions/XKB.h` and friends, it has been substantially reworked to
expose fewer internal details to clients.

See the [API Documentation](https://xkbcommon.org/doc/current/topics.html).

## Tools

<!--! @rawHtml -->
Libxkbcommon has a fair set of <abbr title="Command-Line Interface">CLI</abbr>
<code>tools</code>, grouped under the <code>xkbcli</code> application:
<!--! @endRawHtml -->

<dl>
<dt><code>xkbcli compile-keymap</code></dt>
<dd>Compile an XKB keymap</dd>
<dt><code>xkbcli compile-compose</code></dt>
<dd>Compile a compose file</dd>
<dt><code>xkbcli how-to-type</code></dt>
<dd>Show how to type a given Unicode codepoint</dd>
<dt><code>xkbcli interactive-wayland</code></dt>
<dd>Interactive debugger for XKB keymaps for Wayland</dd>
<dt><code>xkbcli interactive-x11</code></dt>
<dd>Interactive debugger for XKB keymaps for X11</dd>
<dt><code>xkbcli interactive-evdev</code></dt>
<dd>Interactive debugger for XKB keymaps for evdev</dd>
<dt><code>xkbcli dump-keymap-wayland</code></dt>
<dd>Dump a XKB keymap from a Wayland compositor</dd>
<dt><code>xkbcli dump-keymap-x11</code></dt>
<dd>Dump a XKB keymap from a X server</dd>
<dt><code>xkbcli list</code></dt>
<dd>List available layouts and more</dd>
</dl>

It may require the installation of the package `libxkbcommon-tools` or similar
name.

## Dataset

libxkbcommon *does not distribute a keyboard layout dataset itself*, other than
for testing purposes.  The most common dataset is **xkeyboard-config**, which is
used by all current distributions for their X11 XKB data.  Further information
on xkeyboard-config is available at its [homepage][xkeyboard-config-home] and at
its [repository][xkeyboard-config-repo].

The dataset for *Compose* is distributed in [libX11], as part of the X locale
data.

[xkeyboard-config-home]: https://www.freedesktop.org/wiki/Software/XKeyboardConfig
[xkeyboard-config-repo]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
[libX11]: https://gitlab.freedesktop.org/xorg/lib/libx11

## Relation to X11

See [Compatibility](doc/compatibility.md) notes.

## Development

An project’s homepage can be found at https://xkbcommon.org.

xkbcommon is maintained in git at: https://github.com/xkbcommon/libxkbcommon

Patches are always welcome, and may be sent to either
<xorg-devel@lists.x.org> or <wayland-devel@lists.freedesktop.org>
or in a [GitHub](https://github.com/xkbcommon/libxkbcommon) pull request.

Bug reports (and usage questions) are also welcome, and may be filed at
[GitHub](https://github.com/xkbcommon/libxkbcommon/issues).

The maintainers are:
- [Daniel Stone](mailto:daniel@fooishbar.org)
- [Ran Benita](mailto:ran@unusedvar.com)
- [Pierre Le Marre](mailto:dev@wismill.eu)

## Credits

Many thanks are due to Dan Nicholson for his heroic work in getting xkbcommon
off the ground initially.
