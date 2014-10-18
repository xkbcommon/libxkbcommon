# libxkbcommon

xkbcommon is a keymap compiler and support library which processes a
reduced subset of keymaps as defined by the XKB specification.  Primarily,
a keymap is created from a set of Rules/Model/Layout/Variant/Options names,
processed through an XKB ruleset, and compiled into a struct xkb_keymap,
which is the base type for all xkbcommon operations.

From an xkb_keymap, an xkb_state object is created which holds the current
state of all modifiers, groups, LEDs, etc, relating to that keymap.  All
key events must be fed into the xkb_state object using xkb_state_update_key().
Once this is done, the xkb_state object will be properly updated, and the
keysyms to use can be obtained with xkb_state_key_get_syms().

## Quick Guide

See [Quick Guide](doc/quick-guide.md).

## API

While libxkbcommon's API is somewhat derived from the classic XKB API as found
in X11/extensions/XKB.h and friends, it has been substantially reworked to
expose fewer internal details to clients.

See the [API Documentation](http://xkbcommon.org/doc/current/modules.html).

## Dataset

libxkbcommon does not distribute a keymap dataset itself, other than for
testing purposes.  The most common dataset is xkeyboard-config, which is used
by all current distributions for their X11 XKB data.  More information on
xkeyboard-config is available here:
    http://www.freedesktop.org/wiki/Software/XKeyboardConfig

## Relation to X11

See [Compatibility](doc/compat.md) notes.

## Development

An extremely rudimentary homepage can be found at
    http://xkbcommon.org

xkbcommon is maintained in git at
    https://github.com/xkbcommon/libxkbcommon

Patches are always welcome, and may be sent to either
    <xorg-devel@lists.x.org> or <wayland-devel@lists.freedesktop.org>

Bugs are also welcome, and may be reported either at
    Bugzilla https://bugs.freedesktop.org/describecomponents.cgi?product=libxkbcommon
or
    Github https://github.com/xkbcommon/libxkbcommon/issues

The maintainers are
- Daniel Stone <daniel@fooishbar.org>
- Ran Benita <ran234@gmail.com>

## Credits

Many thanks are due to Dan Nicholson for his heroic work in getting xkbcommon
off the ground initially.
