Added the **state event API:**
- `struct xkb_server_state` (new):
  - `xkb_server_state_new()`
  - `xkb_server_state_ref()`
  - `xkb_server_state_unref()`
  - `xkb_server_state_get_keymap()`
  - `xkb_server_state_update_latched_locked()`
  - `xkb_server_state_update_enabled_controls()`
  - `xkb_server_state_update_key()`
- `struct xkb_server_options` (new):
  - `xkb_server_options_new()`
  - `xkb_server_options_destroy()`
  - `xkb_server_options_update_a11y_flags()`
- `struct xkb_events` (new):
  - `enum xkb_events_flags`
  - `xkb_events_new()`
  - `xkb_events_destroy()`
  - `xkb_events_next()`
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
