Enable the configuration of out-of-range layout handling using the
following new API:
- `xkb_state_update::layout_policy`
- `struct xkb_layout_policy_update`
- `enum xkb_layout_out_of_range_policy`, with values:
  - `XKB_LAYOUT_OUT_OF_RANGE_WRAP`: wrap into range using integer
    modulus (default, as before).
  - `XKB_LAYOUT_OUT_OF_RANGE_CLAMP`: clamp into range, i.e.
    invalid indices are corrected to the closest valid bound (0 or
    highest layout index).
  - `XKB_LAYOUT_OUT_OF_RANGE_REDIRECT`: redirect to a specific
    layout index.
