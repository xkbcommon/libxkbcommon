# XKB Compatibility {#xkbcommon-compatibility}

@tableofcontents{html:2}

This page presents the differences between the [XKB 1.0 specification][XKB Protocol]
implemented in current X servers and its implementation in libxkbcommon.

xkbcommon has *removed* support for some parts of the specification which
introduced unnecessary complications.  Many of these removals were in fact
not implemented, or half-implemented at best, as well as being totally
unused in the standard keyboard layout database, [xkeyboard-config].

On the other hand, xkbcommon has notable additions that lift hard-coded
limitation of the [X11 Protocol].

@todo This page is work in progress. It aims to be exhaustive.
Please [report any issue](https://github.com/xkbcommon/libxkbcommon/issues).

[X11 Protocol]: https://www.x.org/releases/current/doc/xproto/x11protocol.html#Keyboards
[XKB Protocol]: https://www.x.org/releases/current/doc/kbproto/xkbproto.html
[xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config

## Keymap support {#keymap-support}

### General features

<table>
<caption>Keymap features compatibility table</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11</th>
<th>xkbcommon (v1 format)</th>
<th>xkbcommon (v2 format)</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<tr>
<th>Wayland support</th>
<td>
<details>
<summary>❌️ No support</summary>
Wayland support requires the XWayland compatibility layer.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
libxkbcommon is the *reference* implementation of the keyboard keymap handling
(parsing/serializing, state) for Wayland.
</details>
</td>
</tr>
<tr>
<th>User configuration</th>
<td>
<details>
<summary>❌️ No support</summary>
Layout database path is *hard-coded* in xserver.

`xkbcomp` enable path configuration, but `setxkbmap` support is incomplete.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Multiple layout database path can be used simultaneously, enabling user-space
configuration.

See @ref user-configuration "" for further information.
</details>
</td>
</tr>
<tr>
<th>Keycode override with aliases</th>
<td>
<details>
<summary>❌️ No support</summary>
Keycodes have always priority over aliases.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.12)</summary>
Keycodes and aliases share the same namespace.
</details>
</td>
</tr>
<tr>
<th>Extended keycodes</th>
<td>
<details>
<summary>❌️ No support</summary>
Limited to **8**-bit keycodes.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Support all Linux keycodes using **32**-bit keycodes.
</details>
</td>
</tr>
<tr>
<th>Extended key names</th>
<td>
<details>
<summary>❌️ No support</summary>
Limited to **4**-character names.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Support any key names of any length.
</details>
</td>
</tr>
<tr>
<th>Extended number of layouts</th>
<td>
<details>
<summary>❌️ No support</summary>
Limited to **4** layouts.
</details>
</td>
<td>
<details>
<summary>❌️ No support</summary>
Limited to **4** layouts.
</details>
</td>
<td>
<details>
<summary>✅ Full support (since 1.11)</summary>
Enable up to **32** layouts when using `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
</tr>
<tr>
<th>Unified modifiers</th>
<td>
<details>
<summary>❌️ No support</summary>
Clear separation between *real* (i.e. core) and *virtual* modifiers.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Real and virtual modifiers have been collapsed into the same namespace, with a
“significant” flag that largely parallels the core/virtual split.

Real modifiers are predefined modifiers with fixed encoding and considered merely
as a X11 compatibility feature.
</details>
</td>
</tr>
<tr>
<th>Extended modifiers</th>
<td>
<details>
<summary>❌️ No support</summary>
Limited to up to **8** *independent* modifiers.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Enable up to **32** *independent* modifiers.
</details>
</td>
</tr>
<tr>
<th>Canonical virtual modifiers</th>
<td>
<details>
<summary>❌️ No support</summary>
Virtual modifiers can only mapped to *real* modifiers (8 bits).
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
Only if using explicit mapping: e.g. `virtual_modifiers M = 0x100;` if `M` has
the modifier index 8.
</details>
</td>
<td>
<details>
<summary>✅ Full support</summary>
Virtual modifiers that are not mapped either *explicitly* (using e.g.
`virtual_modifiers M = …`) or *implicitly* (using `modifier_map` and
`virtualModifier`) [automatically](@ref auto-modifier-encoding) use to
their <em>[canonical mapping](@ref canonical-modifier-def)</em>.
</details>
</td>
</tr>
<tr>
<th>Multiple groups per key</th>
<td>
<details>
<summary>✅ Full support</summary>
Contiguous identical groups are merged together.
</details>
</td>
<td colspan="2">
<details>
<summary>⚠️ Not supported in the [RMLVO] API</summary>
Since version 1.8 the [RMLVO] API does not support parsing multiple groups per
key anymore, because it may break the expectation of most desktop environments and
tools that <em>the number of groups should be equal to the number of configured
layouts</em>.

See @ref how-do-i-define-multiple-groups-per-key "" for migration instructions.
</details>
</td>
</tr>
<tr>
<th>Multiple keysyms per level</th>
<td>
<details>
<summary>❌️ Parsing only</summary>
Ignored: fallback to `NoSymbol`.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
@todo rationale
</details>
</td>
</tr>
<tr>
<th>Multiple actions per level</th>
<td>
<details>
<summary>❌️ No support</summary>
Parse error.
</details>
</td>
<td>
<details>
<summary>⚠️ Parsing & handling, no serialization</summary>
Currently limited to 1 action for each action type “group” and “modifier”.

@since 1.8: Enable multiple actions per level (parsing, serializing & handling).
@since 1.11: Serialize to `VoidAction()` for compatibility with X11.
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
Currently limited to 1 action for each action type “group” and “modifier”.

@since 1.11
</details>
</td>
</tr>
<!-- Removals -->
<tr>
<th>Geometry @anchor geometry-support</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>⚠️ Parsing only</summary>

Rational:
- There were very few geometry definitions available in [xkeyboard-config], and
  while xkbcommon was responsible for parsing this insanely complex format,
  it never actually did anything with it.
- Hopefully someone will develop a companion library which supports keyboard
  geometries in a more useful format, e.g. SVG.
</details>
</td>
</tr>
<tr>
<th>Radio groups</th>
<td>✅ Full support</td><!-- exact status? -->
<td colspan="2">
<details>
<summary>❌️ No support</summary>
Unused in [xkeyboard-config] layouts.
</details>
</td>
</tr>
<tr>
<th>Overlays</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>❌️ No support</summary>
Unused in [xkeyboard-config] layouts.
</details>
</td>
</tr>
<tr>
<th>[Key behaviors]</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>❌️ No support</summary>
Used to implement radio groups and overlays, and to deal with things
like keys that physically lock;

Unused in [xkeyboard-config] layouts.
</details>
</td>
</tr>
<tr>
<th>[Indicator behaviors]</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>⚠️ Partial support</summary>
<!-- See commit 6e676cb7 -->
E.g. LED-controls-key behavior (X11’s `IM_LEDDrivesKB` flag enabled) is not
supported.

The only supported LED behavior is *key-controls-LED*.

Unused in [xkeyboard-config] layouts.
</details>
</td>
</tr>
</tbody>
</table>

[Key behaviors]: https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Key_Behavior
[Indicator behaviors]: https://www.x.org/releases/current/doc/kbproto/xkbproto.html#:~:text=IM_LEDDrivesKB


### Key actions {#compatibility-key-actions}

<table>
<caption>Key actions compatibility table</caption>
<thead>
<tr>
<th>Type</th>
<th>Action</th>
<th>X11</th>
<th>xkbcommon (v1 format)</th>
<th>xkbcommon (v2 format)</th>
</tr>
</thead>
<tbody>
<tr>
<th rowspan="2">Ineffectual</th>
<th>`NoAction()`</th>
<td>✅ Full support</td>
<td colspan="2">✅ Full support</td>
</tr>
<tr>
<th>`VoidAction()`</th>
<td>❌️ No support</td>
<td>⚠️ Parsing only (since 1.10)</td>
<td>✅ Full support (since 1.11)</td>
</tr>
<tr>
<th rowspan="3">Modifiers</th>
<th>`SetModifiers()`</th>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `unlockOnPress` parameter is not supported.
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `unlockOnPress` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
<td>
<details>
<summary>✅ Full support</summary>
- `unlockOnPress` parameter (since 1.11). See @ref set-mods-action "its documentation"
  for further details.
</details>
</td>
</tr>
<tr>
<th>`LatchModifiers()`</th>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `latchOnPress` parameter is not supported.
- `unLockOnPress` parameter is not supported.
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `latchOnPress` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
- `unLockOnPress` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
<td>
<details>
<summary>✅ Full support</summary>
- `latchOnPress` parameter (since 1.11). See @ref latch-mods-action "its documentation"
  for further details.
- `unLockOnPress` parameter (since 1.11). See @ref latch-mods-action "its documentation"
  for further details.
</details>
</td>
</tr>
<tr>
<th>`LockModifiers()`</th>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `unlockOnPress` parameter is not supported.
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `unlockOnPress` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
<td>
<details>
<summary>✅ Full support</summary>
- `unlockOnPress` parameter (since 1.11). See @ref lock-mods-action "its documentation"
  for further details.
</details>
</td>
</tr>
<tr>
<th rowspan="3">Groups</th>
<th>`SetGroup()`</th>
<td>✅ Full support</td>
<td colspan="2">✅ Full support</td>
</tr>
<tr>
<th>`LatchGroup()`</th>
<td>✅ Full support</td>
<td colspan="2">✅ Full support</td>
</tr>
<tr>
<th>`LockGroup()`</th>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `lockOnRelease` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
<td>
<details>
<summary>⚠️ Partial support</summary>
- `lockOnRelease` parameter is not supported. Use `::XKB_KEYMAP_FORMAT_TEXT_V2`.
</details>
</td>
<td>
<details>
<summary>✅ Full support</summary>
- `lockOnRelease` (since 1.11). See @ref lock-group-action "its documentation"
  for further details.
</details>
</td>
</tr>
<tr>
<th rowspan="9">Legacy action</th>
<th>`MovePointer()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`PointerButton()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`LockPointerButton()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`SetPointerDefault()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`SetControls()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`LockControls()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`TerminateServer()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`SwitchScreen()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th>`Private()`</th>
<td>✅ Full support</td>
<td colspan="2">⚠️ Parsing and serializing only, no API support</td>
</tr>
<tr>
<th rowspan="6">Unsupported legacy action</th>
<th>`RedirectKey()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
<tr>
<th>`ISOLock()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
<tr>
<th>`DeviceButton()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
<tr>
<th>`LockDeviceButton()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
<tr>
<th>`DeviceValuator()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
<tr>
<th>`MessageAction()`</th>
<td>✅ Full support</td>
<td colspan="2">❌️ Parsing only</td>
</tr>
</tbody>
</table>


### Keymap text format

<table>
<caption>Keymap text format compatibility table</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11 (xkbcomp)</th>
<th>xkbcommon (v1 format)</th>
<th>xkbcommon (v2 format)</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<tr>
<th>Optional keymap components</th>
<td>❌️ All components are mandatory</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
Keymap components are no longer mandatory, e.g. a keymap without a
`xkb_types` section is legal.
</details>
</td>
</tr>
<tr>
<th>Strong type check</th>
<td>❌️ Weak type check</td>
<td colspan="2">
<details>
<summary>⚠️ Stronger type check (WIP)</summary>
Floating-point numbers cannot be used where an integer is expected.
</details>
</td>
</tr>
<tr>
<th>`replace` merge mode in include statements</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
Supported using the prefix `^`, in addition to the standard *merge* `|` and
*override* `+` modes.
</details>
</td>
</tr>
<tr>
<th>Keysym as strings</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
Keysyms can be written as their corresponding string, e.g. `udiaeresis` can be
written `"ü"`. A string with multiple Unicode code points denotes a list of the
corresponding keysyms. An empty string denotes the keysym `NoSymbol`.
</details>
</td>
</tr>
<tr>
<th>Unicode escape sequence</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
`\u{NNNN}`.

See @ref keymap-string-literal "string literal" for further information.
</details>
</td>
</tr>
<tr>
<th>Extended `GroupN` constants</th>
<td>
<details>
<summary>❌️ No support</summary>
Only `Group1`..`Group8` are supported, although the resulting group must be in
the range 1..4.
</details>
</td>
<td>
<details>
<summary>❌️ No support</summary>
Only `Group1`..`Group4` are supported.

Use `::XKB_KEYMAP_FORMAT_TEXT_V2` in order to support further groups.
</details>
</td>
<td>
<details>
<summary>✅ Full support (since 1.11)</summary>
The pattern `Group<INDEX>` can be used for any valid group index `<INDEX>`.
</details>
</td>
</tr>
<tr>
<th>Extended `LevelN` constants</th>
<td>
<details>
<summary>❌️ No support</summary>
Only `Level1`..`Level8` are supported.
</details>
</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.11)</summary>
Since 1.11, the pattern `Level<INDEX>` can be used for any valid level index
`<INDEX>`.

Before 1.11, only `Level1`..`Level8` were supported.
</details>
</td>
</tr>
<tr>
<th>Extended include</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.11)</summary>
Enable *absolute* paths and *`%`-expansion*.

See @ref keymap-include-percent-expansion "" for further details.
</details>
</td>
</tr>
<!-- Removals -->
<tr>
<th>Include predefined maps</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>❌️ No support</summary>
The modern approach is to use [RMLVO].
</details>
</td>
</tr>
<tr>
<th>Exponent syntax for floating-point numbers</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>❌️ No support</summary>
@todo syntax description
</details>
</td>
</tr>
<tr>
<th>`alternate` merge mode</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>⚠️ Parsing, fallback to default merge mode</summary>
`alternate` was used in `xkb_keycodes` type sections and meant that if a new
keycode name conflicts with an old one, consider it as a keycode *alias*.
</details>
</td>
</tr>
<tr>
<th>Multiple group definition in symbols section</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>⚠️ Supported, except in the [RMLVO] API</summary>
Since 1.8, only 1 group per symbol section is supported in the [RMLVO] API, to
avoid unintuitive results.

Multiple groups per symbol section is supported when parsing a [KcCGST] keymap.
</details>
</td>
</tr>
</tbody>
</table>


### API

<table>
<caption>API compatibility table</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11</th>
<th>xkbcommon (v1 format)</th>
<th>xkbcommon (v2 format)</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<tr>
<th>Full Unicode support</th>
<td>❌️ Incomplete</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
Full support of simple case mappings for `xkb_keysym_to_lower()` and
`xkb_keysym_to_upper()`.
</details>
</td>
</tr>
<!-- TODO -->
<!-- Removals -->
<tr>
<th>[KcCGST] @anchor KcCGST-support</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>⚠️ Partial support (since 1.10)</summary>
- [KcCGST] is considered an implementation detail, use [RMLVO] instead.
- Use `xkb_component_names::xkb_components_names_from_rules()` for
  debugging purposes.
</details>
</td>
</tr>
<tr>
<th>XKM file format</th>
<td>✅ Full support</td>
<td colspan="2">
<details>
<summary>❌️ No support</summary>
Obsolete legacy file format tied to X11 ecosystem.
</details>
</td>
</tr>
<!-- TODO -->
</tbody>
</table>

[KcCGST]: @ref KcCGST-intro
[RMLVO]: @ref RMLVO-intro


## Rules support {#rules-support}

<table>
<caption>API compatibility table</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11</th>
<th>xkbcommon (v1 format)</th>
<th>xkbcommon (v2 format)</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<tr>
<th>`! include` statement</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support</summary>
See @ref rules-include-expansion "rules include statement" for further details.
</details>
</td>
</tr>
<tr>
<th>`replace` merge mode</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
Support the merge mode *replace* via the prefix `^`, in addition to
the standard *merge* `|` and *override* `+` modes.
</details>
</td>
</tr>
<tr>
<th>Extended layout indices</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.8)</summary>
- *single*: matches a single layout; `layout[single]` is the same as without
explicit index: `layout`.
- *first*: matches the first layout/variant, no matter how many layouts are in
the RMLVO configuration. Acts as both `layout` and `layout[1]`.
- *later*: matches all but the first layout. This is an index range. Acts as
`layout[2]` .. `layout[MAX_LAYOUT]`, where `MAX_LAYOUT` is currently 4.
- *any*: matches layout at any position. This is an index range.

- the `:all` qualifier: it applies the qualified item to all layouts.

See @ref rules-extended-layout-indices "extended layout indices" for further
details.
</details>
</td>
</tr>
<tr>
<th>`:all` qualifier</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.8)</summary>
the `:all` qualifier: it applies the qualified item to all layouts.

See @ref rules-all-qualifier ":all qualifier" for further
details.
</details>
</td>
</tr>
<tr>
<th>Extended wild cards</th>
<td>❌️ No support</td>
<td colspan="2">
<details>
<summary>✅ Full support (since 1.9)</summary>
- `<none>` matches *empty* value;
- `<some>` matches *non-empty* value in *every* context.
- `<any>` matches *optionally empty* value in *every* context, contrary to the
  legacy `*` wild card which does not match empty values for layout and variant.

See @ref rules-wildcard-def "rules wildcards" for further information.
</details>
</td>
</tr>
<!-- Removals -->
<!-- TODO -->
</tbody>
</table>


<!-- TODO: tools. Move some of the FAQ entries here? -->


## Keyboard layout registry {#registry-support}


<table>
<caption>Keyboard layout registry support</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11</th>
<th>xkbcommon</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<tr>
<th>XML format</th>
<td>❌️ No support</td>
<td>
<details>
<summary>✅ Full support</summary>
@todo rationale
</details>
</td>
</tr>
<!-- TODO -->
<!-- Removals -->
<!-- TODO -->
</tbody>
</table>


## Compose support {#compose-support}

Relative to the standard implementation in libX11 (described in the
Compose(5) man-page):

<table>
<caption>Compose support</caption>
<thead>
<tr>
<th>Feature</th>
<th>X11 (`libX11`)</th>
<th>xkbcommon</th>
</tr>
</thead>
<tbody>
<!-- Additions -->
<!-- TODO -->
<!-- Removals -->
<tr>
<th>`[!] MODIFIER` syntax</th>
<td>✅ Full support</td>
<td>
<details>
<summary>⚠️ Parsing only</summary>
Syntax: `[([!] ([~] MODIFIER)...) | None] <keysym>`

If the modifier list is preceded by `!` it must match exactly. MODIFIER may be
one of `Ctrl`, `Lock`, `Caps`, `Shift`, `Alt` or `Meta`. Each modifier may be
preceded by a `~` character to indicate that the modifier must not be present.
If `None` is specified, no modifier may be present.

@todo removal rationale
</details>
</td>
</tr>
<tr>
<th>Modifier keysyms in sequences</th>
<td>✅ Full support</td>
<td>
<details>
<summary>⚠️ Parsed, but key events are ignored</summary>
Modifiers should not be used in Compose sequences. Use keymap’s features or
a keysym with more appropriate semantics.
</details>
</td>
</tr>
<tr>
<th>Interactions with Braille keysyms</th>
<td>✅ Full support</td>
<td>
<details>
<summary>❌️ No support</summary>
@todo feature description, removal rationale
</details>
</td>
</tr>
<!-- TODO -->
</tbody>
</table>
