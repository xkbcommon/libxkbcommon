Added the **server machine API:**
- `struct xkb_machine` (new):
  - `xkb_machine_new()`
  - `xkb_machine_ref()`
  - `xkb_machine_unref()`
  - `xkb_machine_get_keymap()`
  - `xkb_machine_process_key()`
  - `xkb_machine_process_synthetic()`
- `struct xkb_machine_options` (new):
  - `xkb_machine_options_new()`
  - `xkb_machine_options_destroy()`
  - `xkb_machine_options_update_a11y_flags()`
- `enum xkb_events_flags` (new)
- `struct xkb_events` (new):
  - `xkb_events_new_batch()`
  - `xkb_events_destroy()`
  - `xkb_events_next()`
- `struct xkb_event` (new):
  - `xkb_event_get_type()`
  - `xkb_event_get_keycode()`
  - `xkb_event_get_components()`
- `struct xkb_state`:
  - `xkb_state_update_event()`

This is the recommended API for **server** applications. It enables the full
feature set that libxkbcommon supports.

This API enables to generate a sequence of [events](@ref xkb_event) corresponding
to *atomic* state changes, contrary to the `xkb_state` API that cannot generate
events. Additionally, the event API supports events other than state
components changes, such as keys events, so that it enables handling most of
the XKB [key actions](@ref key-action-def).

See the [example for a Wayland server](@ref quick-guide-wayland-server)
in the quick guide.
