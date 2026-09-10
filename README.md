# xkbcommon

<!--
NOTE: This file is carefully formatted to support both Github and Doxygen.
They handle line breaks differently!
-->

**xkbcommon** is a keyboard keymap compiler and support library which
processes keymaps as defined by the [XKB] \(X Keyboard Extension) specification
(minus some legacy features). It also contains a module for handling *Compose*
and dead keys, a separate *registry* library for listing available keyboard
layouts and a fair set of <!--!
@rawHtml --><abbr title="Command-Line Interface">CLI</abbr><!--!
@endRawHtml --> *tools* to support keyboard layouts development.

xkbcommon is the standard keymap handling library on Wayland and is used by
compositors, toolkits, and applications to handle keyboard state and translate
key events into characters and actions.

[XKB]: doc/introduction-to-xkb.md

## Quick Guide

### Introduction to XKB

See [Introduction to XKB][XKB] for the essentials of XKB concepts and how a
keymap is built.

### Features

xkbcommon implements <em>most of the [XKB] specification</em>, except for
some obscure features. xkbcommon has *notable additions* that lift hard-coded limitation of the X11 Protocol (e.g. keycodes and modifiers).

See the [Compatibility](doc/compatibility.md) page for further details.

### Using the library

<dl>
<dt>

[Library quick guide](doc/quick-guide.md)
</dt>
<dd>Introduction on how to use this library</dd>
<dt>

[API Documentation](https://xkbcommon.org/doc/current/topics.html)
</dt>
<dd>
Full API reference.

While libxkbcommon’s API is somewhat derived from the classic XKB API as found
in <code>X11/extensions/XKB.h</code> and friends, it has been substantially
reworked to expose fewer internal details to clients.
</dd>
<dt>

[Release notes](doc/release-notes.md)
</dt>
<dd>History of the xkbcommon changes by version</dd>
<dt>

[FAQ](doc/faq.md#faq-api)
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

[Tools](doc/debugging.md)
</dt>
<dd>
<!--! @rawHtml -->
xkbcommon offers a set of <abbr title="Command-Line Interface">CLI</abbr>
tools, grouped under the <code>xkbcli</code> application.
<!--! @endRawHtml -->
</dd>
<dt>

[FAQ](doc/faq.md#faq-keyboard-layouts)
</dt>
<dd>Frequently Asked Questions</dd>
</dl>

## Building

xkbcommon is built with Meson:

```shell
meson setup build
meson compile -C build
meson test -C build
```

See the [building](doc/building.md) guide for dependencies and configuration options.

## Layouts database

xkbcommon *does not distribute a keyboard layout dataset itself*, other than
for testing purposes.  The most common dataset is **xkeyboard-config**, which is
used by all current distributions for their XKB data.  Further information
on xkeyboard-config is available at its [homepage][xkeyboard-config-home] and at
its [repository][xkeyboard-config-repo].

The dataset for *Compose* is distributed in [libX11], as part of the X locale
data.

[xkeyboard-config-home]: https://www.freedesktop.org/wiki/Software/XKeyboardConfig
[xkeyboard-config-repo]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
[libX11]: https://gitlab.freedesktop.org/xorg/lib/libx11

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
