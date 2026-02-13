# Frequently Asked Question (FAQ) {#faq}

@tableofcontents{html:2}

## XKB

### What is XKB?

See: [Introduction to XKB](./introduction-to-xkb.md).

### What does … mean?

See: [terminology](./keymap-text-format-v1-v2.md#terminology).

### What are the differences with the Xorg/X11 implementation?

<dl>
<dt>Features</dt>
<dd>See @ref xkbcommon-compatibility "".</dd>
<dt>Tools</dt>
<dd>See @ref legacy-x-tools-replacement "".</dd>
</dl>

## Keyboard layouts

### Where are the standard keyboard layouts designed?<br/>How to report an issue or propose a patch?

The xkbcommon project does not provide keyboard layouts.
Standard keyboard layouts are provided by the [xkeyboard-config] project.
See [contributing to xkeyboard-config] for further information.

[contributing to xkeyboard-config]: https://xkeyboard-config.freedesktop.org/doc/contributing/

### Where are the system keyboard layouts located?

They are usually located at `/usr/share/xkeyboard-config-2`, or
`/usr/share/X11/xkb` on older setups.

@note Do not modify system files! See @ref how-do-i-customize-my-layout "" for
further instructions.

### Why does my key combination to switch between layouts not work?

There are some issues with *modifier-only* shortcuts: see [this bug report][#420].

This can be fixed by using the new parameter <code>[lockOnRelease]</code> in
`LockGroup()`, available since libxkbcommon 1.11. This will be done at some
point in [xkeyboard-config].

```diff
 xkb_compatibility {
     interpret ISO_Next_Group {
         useModMapMods= level1;
         virtualModifier= AltGr;
-        action= LockGroup(group=+1);
+        action= LockGroup(group=+1, lockOnRelease);
     };
 };
```

[lockOnRelease]: @ref lockOnRelease

### Why do my keyboard shortcuts not work properly?

Users have different expectations when it comes to keyboard shortcuts. However,
the reference use case is usually a single Latin keyboard layout, with fallbacks
for other configurations. These fallbacks may not match users’ expectations nor
even be consistent across applications. See @ref the-keyboard-shortcuts-mess ""
for some examples.

Since version 1.14, libxkbcommon offers a [dedicated API][shortcuts-api]
for *Wayland* compositors, which enables to customize the layouts to use for
keyboard shortcuts.

<dl>
<dt>Issue specific to a *single* application</dt>
<dd>
File a bug report to the corresponding project.
Possible reasons:
- Shortcuts handled with <em>[keycodes]</em>, not <em>[keysyms]</em>.
- Shortcuts handled using only the first layout.
- Shortcuts do not handle non-Latin keyboard layouts.
</dd>
<dt>Issue specific to *multiple* applications</dt>
<dd>
First, check if your desktop environment enables to customize shortcuts handling
*globally*, e.g. by selecting specific layouts when some modifiers are active.
If it does not, consider filing a bug report to your *Wayland* compositor project
to encourage developers to implement the relevant [API][shortcuts-api].
</dd>
</dl>

[shortcuts-api]: @ref xkb_state_machine_options::xkb_state_machine_options_shortcuts_set_mapping
[keycodes]: @ref keycode-def
[keysyms]: @ref keysym-def

[#420]: https://github.com/xkbcommon/libxkbcommon/issues/420

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
<dt>Multiple groups per key do not work</dt>
<dd>See @ref how-do-i-define-multiple-groups-per-key "".</dd>
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

See also the [keymap text format][text format] documentation for the syntax and
the [compatibility](./compatibility.md) page for the supported features.

[text format]: ./keymap-text-format-v1-v2.md

### How do I test my custom layout without installing it?

Use our [debugging tools].

### How do I swap some keys?

🚧 TODO

### How do I define multiple groups per key?

Since version 1.8 the [RMLVO] API does not support parsing multiple groups per
key anymore, because it may break the expectation of most desktop environments and
tools that <em>the number of groups should be equal to the number of configured
layouts</em>. See [#262] and [#518] for further details.

The following explain how to migrate for some common use cases:

<dl>
<dt>Multiple layouts</dt>
<dd>
If you define multiple layouts in a single `xkb_symbols` section, you should
instead split them into individual sections and update your keyboard settings to
use each such layout.

E.g. if you have a single layout `variant1_variant2` defined as:

```c
xkb_symbols "variant1_variant2" {
    key <AD01> {
        symbols[Group1] = [U13AA, U13C6],
        symbols[Group2] = [    q,     Q],
    };
    // …
};
```

then you should split it into:

```c
xkb_symbols "variant1" {
    key <AD01> { [U13AA, U13C6] };
    // …
};

xkb_symbols "variant2" {
    key <AD01> { [    q,     Q] };
    // …
};
```

See also @ref user-configuration "" to make the layouts discoverable for easy
configuration in your keyboard settings app.
</dd>
<dt>Option for multiple layouts</dt>
<dd>
If you define an option that affect multiple groups at once in a single
`xkb_symbols` section, you should split that section and update the corresponding
rules for each layout index.

E.g. if one has the following group switcher on CapsLock key:

```c
// File: ~/.config/xkb/symbols/group
partial xkb_symbols "caps_pairs" {
    replace key <CAPS> {
        repeat=No,
        symbols[Group1] = [ISO_Group_Lock, ISO_Group_Lock],
        symbols[Group2] = [ISO_Group_Lock, ISO_Group_Lock],
        symbols[Group3] = [ISO_Group_Lock, ISO_Group_Lock],
        symbols[Group4] = [ISO_Group_Lock, ISO_Group_Lock],
        actions[Group1] = [LockGroup(group=2), LockGroup(group=3) ],
        actions[Group2] = [LockGroup(group=1), LockGroup(group=4) ],
        actions[Group3] = [LockGroup(group=4), LockGroup(group=1) ],
        actions[Group4] = [LockGroup(group=3), LockGroup(group=2) ]
    };
};
```

and the corresponding custom rules:

```
// File: ~/.config/xkb/rules/evdev
include %S/evdev

! option         = symbols
  grp:caps_pairs = +group(caps_pairs)
```

then it should be migrated to:

```c
// File: ~/.config/xkb/symbols/group
partial xkb_symbols "caps_pairs_1" {
    replace key <CAPS> {
        repeat=No,
        symbols[Group1] = [ISO_Group_Lock, ISO_Group_Lock],
        actions[Group1] = [LockGroup(group=2), LockGroup(group=3) ]
    };
};
partial xkb_symbols "caps_pairs_2" {
    replace key <CAPS> {
        symbols[Group1] = [ISO_Group_Lock, ISO_Group_Lock],
        actions[Group1] = [LockGroup(group=1), LockGroup(group=4) ]
    };
};
partial xkb_symbols "caps_pairs_3" {
    replace key <CAPS> {
        symbols[Group1] = [ISO_Group_Lock, ISO_Group_Lock],
        actions[Group1] = [LockGroup(group=4), LockGroup(group=1) ]
    };
};
partial xkb_symbols "caps_pairs_4" {
    replace key <CAPS> {
        symbols[Group1] = [ISO_Group_Lock, ISO_Group_Lock],
        actions[Group1] = [LockGroup(group=3), LockGroup(group=2) ]
    };
};
```

with the corresponding custom rules:

```
// File: ~/.config/xkb/rules/evdev
include %S/evdev

! layout[1]   option         = symbols
  *           grp:caps_pairs = +group(caps_pairs_1):1

! layout[2]   option         = symbols
  *           grp:caps_pairs = +group(caps_pairs_2):2

! layout[3]   option         = symbols
  *           grp:caps_pairs = +group(caps_pairs_3):3

! layout[4]   option         = symbols
  *           grp:caps_pairs = +group(caps_pairs_4):4
```
</dd>
</dl>

[#262]: https://github.com/xkbcommon/libxkbcommon/issues/262
[#518]: https://github.com/xkbcommon/libxkbcommon/pull/518

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

#### How to get the virtual modifier encoding?

The [virtual modifiers] encoding, (also: mappings to [real modifiers] in X11
jargon) is an implementation detail.
However, some applications may require it in order to interface with legacy code.

##### libxkbcommon ≥ 1.10

Use the dedicated functions `xkb_keymap::xkb_keymap_mod_get_mask()` (since 1.10)
and `xkb_keymap::xkb_keymap_mod_get_mask2()` (since 1.11).

##### libxkbcommon ≤ 1.9

Use the following snippet:

```c
// Find the real modifier mapping of the virtual modifier `LevelThree`
#include <xkbcommon/xkbcommon.h>
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

#### How to get the keys that trigger modifiers?

There is no dedicated API, since the use cases are too diverse or niche.
Nevertheless, the following snippet provide a minimal example to achieve it.

@snippet "test/modifiers.c" xkb_keymap_mod_get_codes

#### How to use keyboard shortcuts from a different layout?

##### The keyboard shortcuts mess

Keyboard shortcuts in applications are usually using Latin letters. In order to
make them work on non-Latin keyboard layouts, each toolkit (Gtk, Qt, etc.)
implements its strategy.

However there are some edge cases where this does not work:

- Strategies may differ slightly, resulting in different shortcuts behavior
  depending on the app used.
- Punctuation is usually not remapped. E.g. the standard Israeli
  layout cannot remap its key `<AD01>` “slash” to the US layout “q”.
- If one have multiple Latin layouts, shortcuts may be positioned
  differently. E.g. US Qwerty and French Azerty have different
  positions for Ctrl+Q, while the user might want all shortcuts to
  be positioned independently of the layout.

##### XKB limitations

Some users want explicitly to use keyboard shortcuts as if they were typing on
another keyboard layout, e.g. using Qwerty shortcuts with a Dvorak layout. While
achievable with modern XKB features (e.g. multiple actions per level), this is
non-trivial and it does not scale well, thus preventing support in the standard
keyboard database, [xkeyboard-config].

##### Custom and consistent shortcuts behavior using libxkbcommon

Since libxkbcommon 1.14, tweaking the keyboard shortcuts can be achieved by using
the following functions from the [state event API]:

<dl>
<dt>
`xkb_state_machine_options::xkb_state_machine_options_shortcuts_update_mods()`
</dt>
<dd>
Set the modifiers that will trigger the shortcuts tweak, typically
`Control+Alt+Super`.

```c
const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
const xkb_mod_mask_t super = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SUPER);
const xkb_mod_mask_t shortcuts_mask = ctrl | alt | super;
if (xkb_state_machine_options_shortcuts_update_mods(options, shortcuts_mask, shortcuts_mask)) {
    /* handle error */
    …
}
```
</dd>
<dt>
`xkb_state_machine_options::xkb_state_machine_options_shortcuts_set_mapping()`
</dt>
<dd>
Set the layout to use for shortcuts for each relevant layout. There are 2 typical
use cases:

<dl>
<dt>*Single* layout</dt>
<dd>
The user types with a single layout, but want the shortcuts to act as if using
another layout: e.g. Qwerty shortcuts for the Arabic layout. The keymap would
be configured with *2* layouts: the user layout then the shortcut layout (e.g.
`ara,us`).

```c
if (xkb_state_machine_options_shortcuts_set_mapping(options, 0, 1)) {
    /* handle error */
    …
}
```
</dd>
<dt>*Multiple* layouts</dt>
<dd>
The user types with multiples layouts but wants shortcuts consistency accross
all the layouts, typically using the first layout as the reference.

```c
// When using shortcuts, all layouts will behave as if using the *first* layout.
const xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(keymap);
for (xkb_layout_index_t source = 1; source < num_layouts; source++) {
    if (xkb_state_machine_options_shortcuts_set_mapping(options, source, 0)) {
        /* handle error */
        …
    }
}
```
</dd>
</dl>

</dd>
</dl>

[state event API]: @ref server-client-state

### Keys

#### How to check if a keymap defines/binds a keycode?

<dl>
<dt>Check if a keymaps *defines* a keycode<dt>
<dd>
`xkb_keymap::xkb_keymap_key_get_name()` returns `NULL` if the keycode is not
defined in the corresponding keymap:

```c
if (xkb_keymap_key_get_name(keymap, keycode) != NULL)
    // use existing key ...
```
</dd>
<dt>Check if a keymaps *binds* a keycode<dt>
<dd>
`xkb_keymap::xkb_keymap_num_layouts_for_key()` returns `0` if the keycode is
either not defined or unbound:

```c
if (xkb_keymap_num_layouts_for_key(keymap, keycode))
    // use bound key ...
```
</dd>
</dl>
