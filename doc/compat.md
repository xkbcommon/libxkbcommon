# Compatibility

Relative to the XKB 1.1 specification implemented in current X servers,
xkbcommon has removed support for some parts of the specification which
introduced unnecessary complications.  Many of these removals were in fact
not implemented, or half-implemented at best, as well as being totally
unused in the standard dataset.

Notable removals:
- geometry support
  + there were very few geometry definitions available, and while
    xkbcommon was responsible for parsing this insanely complex format,
    it never actually did anything with it
  + hopefully someone will develop a companion library which supports
    keyboard geometries in a more useful format
- KcCGST (keycodes/compat/geometry/symbols/types) API
  + use RMLVO instead; KcCGST is now an implementation detail
  + including pre-defined keymap files
- XKM support
  + may come in an optional X11 support/compatibility library
- around half of the interpret actions
  + pointer device, message and redirect actions in particular
- non-virtual modifiers
  + core and virtual modifiers have been collapsed into the same
    namespace, with a 'significant' flag that largely parallels the
    core/virtual split
- radio groups
  + completely unused in current keymaps, never fully implemented
- overlays
  + almost completely unused in current keymaps
- key behaviors
  + used to implement radio groups and overlays, and to deal with things
    like keys that physically lock; unused in current keymaps
- indicator behaviours such as LED-controls-key
  + the only supported LED behaviour is key-controls-LED; again this
    was never really used in current keymaps

On the other hand, some features and extensions were added.

Notable additions:
- 32-bit keycodes
- extended number of modifiers
- extended number of groups
- multiple keysyms per level
  + this requires incompatible dataset changes, such that X11 would
    not be able to parse these
