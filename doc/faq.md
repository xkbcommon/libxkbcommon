# Frequently Asked Question (FAQ) {#faq}

## XKB

### What is XKB?

See: [Introduction to XKB](./introduction-to-xkb.md).

### What does … mean?

See: [terminology](./keymap-format-text-v1.md#terminology).

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
- If you want to modify the standard keyboard layout database, please
  first try it *locally* (see previous) and then file an issue or a merge request
  at the [xkeyboard-config] project.

See also the [keymap text format][text format] documentation for the syntax.

[text format]: ./keymap-format-text-v1.md

### How do I swap some keys?

🚧 TODO

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
