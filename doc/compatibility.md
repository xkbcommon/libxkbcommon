# Compatibility {#xkbcommon-compatibility}

@tableofcontents{html:2}

## Keymap text format v1: XKB 1.0 support {#xkb-v1-compatibility}

Relative to the XKB 1.0 specification implemented in current X servers,
xkbcommon has removed support for some parts of the specification which
introduced unnecessary complications.  Many of these removals were in fact
not implemented, or half-implemented at best, as well as being totally
unused in the standard dataset.

### Notable removals

- Keymap components are no longer mandatory, e.g. a keymap without a
  `xkb_types` section is legal.
- geometry support @anchor geometry
  @anchor geometry-support
  + there were very few geometry definitions available, and while
    xkbcommon was responsible for parsing this insanely complex format,
    it never actually did anything with it
  + hopefully someone will develop a companion library which supports
    keyboard geometries in a more useful format
- KcCGST (keycodes/compat/geometry/symbols/types) API
  @anchor KcCGST-support
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
- xkbcommon has stronger type checks, so floating-point numbers cannot
  be used where an integer is expected.
- exponent syntax for floating-point numbers is not supported.
- legacy merge mode `alternate` is not supported and is ignored.
- support the merge mode *replace* in include statements via the
  prefix `^`, in addition to the standard *merge* `|` and *override*
  `+` modes.

On the other hand, some features and extensions were added.

### Notable additions

- 32-bit keycodes
- extended number of modifiers (planned)
- extended number of groups (planned)
- multiple keysyms and actions per level
  + such levels are ignored by x11/xkbcomp.
- key names (e.g. `<AE11>`) can be longer than 4 characters.
- keysyms can be written as their corresponding string, e.g. `udiaeresis`
  can be written `"ü"`. A string with multiple Unicode code points denotes
  a list of the corresponding keysyms. An empty string denotes the keysym
  `NoSymbol`.

## Keymap text format v2 {#xkb-v2-compatibility}

@todo

## Rules support {#rules-support}

### Additions

- `! include` statement.
- Support the merge mode *replace* via the prefix `^`, in addition to
  the standard *merge* `|` and *override* `+` modes.
- Additional wild cards:
  - `<none>` matches *empty* value;
  - `<some>` matches *non-empty* value in *every* context.
  - `<any>` matches *optionally empty* value in *every* context, contrary to the
    legacy `*` wild card which does not match empty values for layout and
    variant;

## Compose support {#compose-support}

Relative to the standard implementation in libX11 (described in the
Compose(5) man-page), some features are not supported:

- the (! MODIFIER) syntax
    + parsed correctly but ignored.
- using modifier keysyms in Compose sequences
- several interactions with Braille keysyms
