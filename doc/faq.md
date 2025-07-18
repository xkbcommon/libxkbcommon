# Frequently Asked Question (FAQ) {#faq}

@tableofcontents{html:2}

## XKB

### What is XKB?

See: [Introduction to XKB](./introduction-to-xkb.md).

### What does … mean?

See: [terminology](./keymap-text-format-v1-v2.md#terminology).

## Keyboard layout

### Where are the keyboard layouts defined?

The xkbcommon project does not provide keyboard layouts.
See the [xkeyboard-config] project for further information.

### Why do my keyboard shortcuts not work properly?

- 🚧 TODO: Setups with multiple layout and/or non-Latin keyboard layouts may have some
  issues.
- 🚧 TODO: [#420]

[#420]: https://github.com/xkbcommon/libxkbcommon/issues/420

### Why does my key combination to switch between layouts not work?

See [this issue][#420].

### Why does my keyboard layout not work as expected?

There could be many reasons!

<dl>
<dt>There is an issue with your keyboard layout database</dt>
<dd>
libxkbcommon may not be able to load your configuration due to an issue
(file not found, syntax error, unsupported keysym, etc.). Please use our
[debugging tools] to get further information.

Note that the xkbcommon project does not provide keyboard layouts.
See the [xkeyboard-config] project for further information.
</dd>
<dt>Diacritics/accents do not work</dt>
<dd>
This is most probably an issue with your *Compose* configuration.
If you customized it, do not forget to restart your session before trying it.

Please use our [debugging tools] with the option `--enable-compose` to get
further information.
</dd>
<dt>The application you use does not handle the keyboard properly</dt>
<dd>
Please use our [debugging tools] to ensure that it is specific to the
application.
</dd>
<dt>Your keyboard layout uses features not supported by libxkbcommon</dt>
<dd>See: [compatibility](./compatibility.md)</dd>
<dt>None of the previous</dt>
<dd>
If none of the previous is conclusive, then this may an issue with libxkbcommon.
Please use our [debugging tools] to provide the maximum information (setup,
log, expected/got results) and file a [bug report]!
</dd>
</dl>

[debugging tools]: ./debugging.md
[xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
[bug report]: https://github.com/xkbcommon/libxkbcommon/issues/new

### How do I customize my layout?

This project does not provide any keyboard layout database:
- If you want to modify only your *local* keyboard configuration,
  see: [User-configuration](./user-configuration.md).
- If you want to modify the standard keyboard layout database, please first try
  it *locally* (see our [debugging tools]) and then file an issue or a merge
  request at the [xkeyboard-config] project.

See also the [keymap text format][text format] documentation for the syntax.

[text format]: ./keymap-text-format-v1-v2.md

### How do I test my custom layout without installing it?

Use our [debugging tools].

### How do I swap some keys?

🚧 TODO

### How do I break a latch before triggering another latch or lock?

Consider the following use cases:
1. If `Caps_Lock` is on the second level of some key, and `Shift` is
   latched, pressing the key locks `Caps` while also breaking the `Shift`
   latch, ensuring that the next character is properly uppercase.
2. On the German E1 layout, `ISO_Level5_Latch` is on the third level
   of `<AC04>`. So if a level 3 latch (typically on `<RALT>`) is used
   to access it, the level 5 must break the previous level 3 latch,
   else both latches would be active: the effective level would be 7
   instead of the intended 5.

Both uses cases can be implemented using the following features:
- explicit action;
- multiple actions per level;
- `VoidAction()`: to break latches.

Patch that fixes the first use case:

```diff
--- old
+++ new
  key <LFSH> {
      [ISO_Level2_Latch, Caps_Lock],
+     [LatchMods(modifiers=Shift,latchToLock,clearLocks),
+      {VoidAction(), LockMods(modifiers=Lock)}],
      type=\"ALPHABETIC\"
  };
```

## Legacy X tools replacement

### xmodmap

<dl>
<dt>`xmodmap -pm`</dt>
<dd>

There is no strict equivalent. Since 1.10 `xkbcli compile-keymap` has the option
`--modmaps` to print the modifiers maps from a keymap, but it does not print
keysyms. In order to get the output for the current keymap, use it with
`xkbcli dump-keymap-*`:

<dl>
<dt>Automatic session type detection</dt>
<dd>

```bash
xkbcli dump-keymap | xkbcli compile-keymap --modmaps
```
</dd>
<dt>Wayland session</dt>
<dd>

```bash
xkbcli dump-keymap-wayland | xkbcli compile-keymap --modmaps
```
</dd>
<dt>X11 session / XWayland</dt>
<dd>

```bash
xkbcli dump-keymap-x11 | xkbcli compile-keymap --modmaps
```
</dd>
</dl>
</dd>
<dt>`xmodmap -e "…"`</dt>
<dt>`xmodmap /path/to/file`</dt>
<dd>No equivalent: `xkbcli` does not modify the display server keymap.</dd>
</dl>

### setxkbmap

<dl>
<dt>`setxkbmap -print -layout …`<dt>
<dd>

Since 1.9 one can use the `--kccgst` option:

```bash
xkbcli compile-keymap --kccgst --layout …
```
</dd>
<dt>`setxkbmap -query`</dt>
<dd>

No equivalent: `xkbcli` only query *raw* keymaps and has no access to the
original [RMLVO] settings.
</dd>
<dt>`setxkbmap -layout …`</dt>
<dd>
No equivalent: `xkbcli` does not modify the display server keymap.
One must use the tools *specific* to each display server in order order to
achieve it.
<!-- TODO: links to doc of most important DE -->

If you use a custom layout, please have a look at @ref user-configuration "",
which enables making custom layouts *discoverable* by keyboard configuration GUI.
</dd>
</dl>

### xkbcomp

<dl>
<dt>`xkbcomp -xkb /path/to/keymap/file -`</dt>
<dd>

```bash
xkbcli compile-keymap --keymap /path/to/keymap/file
```
</dd>
<dt>`xkbcomp -xkb $DISPLAY -`</dt>
<dd>

<dl>
<dt>Automatic session type detection</dt>
<dd>

```bash
xkbcli dump-keymap
```
</dd>
<dt>Wayland session</dt>
<dd>

```bash
xkbcli dump-keymap-wayland
```
</dd>
<dt>X11 session</dt>
<dd>

```bash
xkbcli dump-keymap-x11
```
</dd>
</dl>
</dd>
<dt>`xkbcomp - $DISPLAY`</dt>
<dt>`xkbcomp /path/to/keymap/file $DISPLAY`</dt>
<dd>

No equivalent: `xkbcli` does not modify the display server keymap.
One must use the tools *specific* to each display server in order order to
achieve it. Please have a look at @ref user-configuration "", which enables
making custom layouts *discoverable* by keyboard configuration GUI.
</dd>
</dl>

### xev

<dl>
<dt>`xev -event keyboard`</dt>
<dd>
<dl>
<dt>Automatic session type detection</dt>
<dd>

```bash
xkbcli interactive
```
</dd>
<dt>Wayland session</dt>
<dd>

```bash
xkbcli interactive-wayland
```
</dd>
<dt>X11 session</dt>
<dd>

```bash
xkbcli interactive-x11
```
</dd>
</dl>
</dd>
</dl>

## API

### Modifiers

#### How to get the virtual to real modifiers mappings?

The [virtual modifiers] mappings to [real modifiers] is an implementation detail.
However, some applications may require it in order to interface with legacy code.

##### libxkbcommon ≥ 1.10

Use the dedicated function `xkb_keymap::xkb_keymap_mod_get_mask()`.

##### libxkbcommon ≤ 1.9

Use the following snippet:

```c
// Find the real modifier mapping of the virtual modifier `LevelThree`
#include <xkbcommon/xkbcommon-names.h>
const xkb_mod_index_t levelThree_idx = xkb_keymap_mod_get_index(keymap, XKB_VMOD_NAME_LEVEL3);
const xkb_mod_mask_t levelThree = UINT32_C(1) << levelThree_idx;
struct xkb_state* state = xkb_state_new(keymap);
assert(state); // Please handle error properly
xkb_state_update_mask(state, levelThree, 0, 0, 0, 0, 0);
const xkb_mod_mask_t levelThree_mapping = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
xkb_state_unref(state);
```

[virtual modifiers]: @ref virtual-modifier-def
[real modifiers]: @ref real-modifier-def
[RMLVO]: @ref RMLVO-intro
