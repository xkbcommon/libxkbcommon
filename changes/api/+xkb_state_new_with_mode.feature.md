Added `xkb_state::xkb_state_new_with_mode()` and the corresponding
`xkb_state_mode` enumeration. They enable creating `xkb_state` objects
with smaller memory footprint and protect against state corruption by
disallowing mixing server and client APIs.
