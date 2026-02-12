Added the **state event API:**
- `struct xkb_state_machine` (new):
  - `xkb_state_machine_new()`
  - `xkb_state_machine_ref()`
  - `xkb_state_machine_unref()`
  - `xkb_state_machine_get_keymap()`
  - `xkb_state_machine_update_latched_locked()`
  - `xkb_state_machine_update_controls()`
  - `xkb_state_machine_update_key()`
- `struct xkb_state_machine_options` (new):
  - `xkb_state_machine_options_new()`
  - `xkb_state_machine_options_destroy()`
  - `xkb_state_machine_options_update_a11y_flags()`
- `struct xkb_event_iterator` (new):
  - `enum xkb_event_iterator_flags`
  - `xkb_event_iterator_new()`
  - `xkb_event_iterator_destroy()`
  - `xkb_event_iterator_next()`
- `struct xkb_event` (new):
  - `xkb_event_get_type()`
  - `xkb_event_get_keycode()`
  - `xkb_event_get_components()`
- `struct xkb_state`:
  - `xkb_state_update_from_event()`

This is the recommended API for **server** applications. It enables the full
feature set that libxkbcommon supports.

This API enables to generate a sequence of [events](@ref xkb_event) corresponding
to *atomic* state changes, contrary to the `xkb_state` API that cannot generate
events. Additionally, the event API supports events other than state
components changes, such as keys events, so that it enables to handle most of
the XKB [key actions](@ref key-action-def).

See the [example for a Wayland server](@ref quick-guide-wayland-server)
in the quick guide.
