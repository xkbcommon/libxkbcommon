Added a new keymap compile options API:
- `xkb_keymap_compile_options_new`
- `xkb_keymap_compile_options_free`
- `xkb_keymap_new_from_names2`
- `xkb_keymap_new_from_file2`
- `xkb_keymap_new_from_buffer2`

This interface allows to configure keymap options that cannot be passed as flags.
