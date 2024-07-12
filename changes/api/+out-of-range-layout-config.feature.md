Enable the configuration of out-of-range layout handling using 2 new keymap compile flags:
- `XKB_KEYMAP_REDIRECT_OUT_OF_RANGE_LAYOUT`: redirect to a specific layout index.
- `XKB_KEYMAP_CLAMP_OUT_OF_RANGE_LAYOUT`: clamp into range, i.e. invalid indexes are
  corrected to the closest valid bound (0 or highest layout index).

When not specified, invalid groups are brought into range using integer modulus.
