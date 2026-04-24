# libxkbcommon

<!--
NOTE: This file is carefully formatted to support both Github and Doxygen.
They handle line breaks differently!
-->

**libxkbcommon** is a keyboard keymap compiler and support library which
processes keymaps as defined by the [XKB] \(X Keyboard Extension) specification
(minus some legacy features). It also contains a module for handling *Compose*
and dead keys, a separate *registry* library for listing available keyboard
layouts and a fair set of <!--!
@rawHtml --><abbr title="Command-Line Interface">CLI</abbr><!--!
@endRawHtml --> *tools* to support keyboard layouts development.

libxkbcommon is the standard keymap handling library on Wayland and is used by
compositors, toolkits, and applications to handle keyboard state and translate
key events into characters and actions.

[XKB]: doc/introduction-to-xkb.md

## Quick Guide

### Introduction to XKB

See [Introduction to XKB][XKB] for the essentials of XKB concepts and how a
keymap is built.

### Using the library

<dl>
<dt>

[Library quick guide](doc/quick-guide.md)
</dt>
<dd>Introduction on how to use this library</dd>
<dt>

[API Documentation](https://xkbcommon.org/doc/current/topics.html)
</dt>
<dd>Full API reference</dd>
<dt>

[Release notes](doc/release-notes.md)
</dt>
<dd>History of the libxkbcommon changes by version</dd>
<dt>

[FAQ](doc/faq.md#api)
</dt>
<dd>Frequently Asked Questions</dd>
</dl>

### Developing keyboard layouts

<dl>
<dt>

[Custom layouts](doc/custom-configuration.md)
</dt>
<dd>Quick start for adding a *custom layout* or option</dd>
<dt>

[Keymap format](doc/keymap-text-format-v1-v2.md)
</dt>
<dd>Documentation of keymap components and keymap text syntax</dd>
<dt>

[Tools](./README.md#tools)
</dt>
<dd>Introduction to the `xkbcli` application</dd>
<dt>

[FAQ](doc/faq.md)
</dt>
<dd>Frequently Asked Questions</dd>
</dl>

## Building

libxkbcommon requires:

- a C compiler supporting C11
- XKB registry (optional): `libxml2`
- X11 features (optional): `libxcb` and `libxcb-xkb`
- Wayland features (optional): `wayland-client`, `wayland-protocols`, `wayland-scanner`

libxkbcommon is built with [Meson](http://mesonbuild.com):

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
@include meson.options
</details>

## API

While libxkbcommon’s API is somewhat derived from the classic XKB API as found
in `X11/extensions/XKB.h` and friends, it has been substantially reworked to
expose fewer internal details to clients.

See the [API Documentation](https://xkbcommon.org/doc/current/topics.html).

## Tools

<!--! @rawHtml -->
Libxkbcommon offers a set of <abbr title="Command-Line Interface">CLI</abbr>
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
<dd>Dump an XKB keymap from a Wayland compositor</dd>
<dt><code>xkbcli dump-keymap-x11</code></dt>
<dd>Dump an XKB keymap from an X server</dd>
<dt><code>xkbcli list</code></dt>
<dd>List available layouts and more</dd>
<dt><code>xkbcli info</code></dt>
<dd>Print information about libxkbcommon configuration, for debugging purposes</dd>
</dl>

It may require the installation of the package `libxkbcommon-tools` or similar
name.

## Layouts database

libxkbcommon *does not distribute a keyboard layout dataset itself*, other than
for testing purposes.  The most common dataset is **xkeyboard-config**, which is
used by all current distributions for their XKB data.  Further information
on xkeyboard-config is available at its [homepage][xkeyboard-config-home] and at
its [repository][xkeyboard-config-repo].

The dataset for *Compose* is distributed in [libX11], as part of the X locale
data.

[xkeyboard-config-home]: https://www.freedesktop.org/wiki/Software/XKeyboardConfig
[xkeyboard-config-repo]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
[libX11]: https://gitlab.freedesktop.org/xorg/lib/libx11

## Compatibility with X11

libxkbcommon is compatible with X11, except for some obscure features.

See the [Compatibility](doc/compatibility.md) page for further details.

## Development

<dl>
<dt>Project’s homepage</dt>
<dd>https://xkbcommon.org</dd>
<dt>Repository</dt>
<dd>https://github.com/xkbcommon/libxkbcommon</dd>
<dt>Contributions</dt>
<dd>

Patches are always welcome, and may be filed at:

- [GitHub](https://github.com/xkbcommon/libxkbcommon) pull request (*preferred*)
- <wayland-devel@lists.freedesktop.org>
</dd>
<dt>Bug reports, questions</dt>
<dd>

Bug reports and questions are also welcome, and may be filed at
[GitHub](https://github.com/xkbcommon/libxkbcommon/issues).

For general topics, prefer opening a [discussion].

[discussion]: https://github.com/xkbcommon/libxkbcommon/discussions
</dd>
<dt>Maintainers</dt>
<dd>

- [Pierre Le Marre](mailto:dev@wismill.eu)
- [Ran Benita](mailto:ran@unusedvar.com)
- [Daniel Stone](mailto:daniel@fooishbar.org)
</dd>
</dl>

## License

See the [LICENSE](doc/license.md) file.

## Credits

Many thanks are due to Dan Nicholson for his heroic work in getting xkbcommon
off the ground initially.
