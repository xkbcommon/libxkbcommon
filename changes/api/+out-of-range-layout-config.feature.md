Enable the configuration of out-of-range layout handling using the new function
`xkb_keymap_compile_options_set_layout_out_of_range_action` and the corresponding
new enumeration `xkb_keymap_out_of_range_layout_action`:
- `XKB_KEYMAP_WRAP_OUT_OF_RANGE_LAYOUT`: wrap into range using integer modulus (default).
- `XKB_KEYMAP_REDIRECT_OUT_OF_RANGE_LAYOUT`: redirect to a specific layout index.
- `XKB_KEYMAP_CLAMP_OUT_OF_RANGE_LAYOUT`: clamp into range, i.e. invalid indexes are
  corrected to the closest valid bound (0 or highest layout index).

When not specified, invalid groups are brought into range using integer modulus.
