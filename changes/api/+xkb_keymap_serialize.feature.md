Added `xkb_keymap::xkb_keymap_serialize()`,  `struct xkb_keymap_serialize_config` and
`struct xkb_serialize_result` to enable more control on serializing or its result.

In particular, it enables to serialize a keymap with more than 4 layouts into
an <strong>X11</strong>-compatible keymap, suitable to use in the [Wayland] protocol
(see: [`wl_keyboard::keymap_format::xkb_v1`](https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format:entry:xkb_v1)).

[Wayland]: https://wayland.freedesktop.org/
