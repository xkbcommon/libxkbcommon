Added API to change controls of the keyboard state:
- new enumeration `xkb_keyboard_control_flags`
- new function `xkb_state_update_synthetic()`
- new function `xkb_state_serialize_enabled_controls()`
- new member `XKB_STATE_CONTROLS` in the `xkb_state_component` enumeration.
