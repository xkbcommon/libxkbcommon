Added API to change controls of the keyboard state:
- new enumeration `xkb_state_controls`
- new function `xkb_state_update_locked_controls()`
- new function `xkb_state_serialize_controls()`
- new member `XKB_STATE_CONTROLS_EFFECTIVE` in the `xkb_state_component`
  enumeration.
