Enable the configuration of out-of-range layout handling using the
following new API:
- `state_machine::xkb_state_machine_update_control()`
- `enum xkb_keyboard_control_param`, with values:
  - `XKB_KEYBOARD_CONTROL_OUT_OF_RANGE_LAYOUT_POLICY`
  - `XKB_KEYBOARD_CONTROL_OUT_OF_RANGE_LAYOUT_REDIRECT`
- `enum xkb_out_of_range_layout_policy`, with values:
  - `XKB_OUT_OF_RANGE_LAYOUT_WRAP`: wrap into range using integer
    modulus (default, as before).
  - `XKB_OUT_OF_RANGE_LAYOUT_SATURATE`: clamp into range, i.e.
    invalid indices are corrected to the closest valid bound (0 or
    highest layout index).
  - `XKB_OUT_OF_RANGE_LAYOUT_REDIRECT`: redirect to a specific
    layout index.
