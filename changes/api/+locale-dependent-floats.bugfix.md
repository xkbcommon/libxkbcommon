Fixed floating-point number parsing failling on locales that use a decimal
separator different than the period `.`.

Note that the issue is unlikely to happen even with such locales, unless
*parsing* a keymap with a *geometry* component which contains floating-point
numbers.

Affected API:
- `xkb_keymap_new_from_file()`,
- `xkb_keymap_new_from_buffer()`,
- `xkb_keymap_new_from_string()`.

Unaffected API:
- `xkb_keymap_new_from_names()`: none of the components loaded use
  floating-point number. libxkbcommon does not load *geometry* files.
- `libxkbcommon-x11`: no such parsing is involved.
