*Modifiers masks* handling has been refactored to properly handle virtual
modifiers. Modifier masks are now always considered as an *opaque encoding* of
the modifiers state:
- Modifiers masks should not be interpreted by other means than the provided API.
  In particular, one should not assume that modifiers masks always denote the
  modifiers *indexes* of the keymap.
- It enables using virtual modifiers with arbitrary mappings. E.g. one can now
  reliably create virtual modifiers without relying on the legacy X11 mechanism,
  that requires a careful use of keys’ real and virtual modmaps.
- It enables *interoperability* with further implementations of XKB.
