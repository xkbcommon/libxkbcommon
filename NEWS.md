libxkbcommon 1.7.0 - 2024-03-24
==================

API
---

### New

- Added early detection of invalid encodings and BOM for keymaps, rules & Compose.
  Also added a hint that the expected encoding must be UTF-8 compatible.

### Fixes

- Updated keysyms using latest [xorgproto] (commit: `cd33097fc779f280925c6d6bbfbd5150f93ca5bc`):

  For the sake of compatibility, this reintroduces some deleted keysyms and
  postpones the effective deprecation of others, that landed in xkbcommon 1.6.0.

  - Additions (reverted removal):

    - `XKB_KEY_dead_lowline`
    - `XKB_KEY_dead_aboveverticalline`
    - `XKB_KEY_dead_belowverticalline`
    - `XKB_KEY_dead_longsolidusoverlay`

  - The following keysyms names remain deprecated, but are set again (i.e. as
    before xkbcommon 1.6.0) as the reference names for their respective keysyms,
    in order to ensure the transition to the newer names that replace them. This
    affects functions such as `xkb_keymap_key_get_name` and `xkb_keymap_get_as_string`.

    - `XKB_KEY_masculine`: is deprecated in favor of `XKB_KEY_ordmasculine`
    - `XKB_KEY_guillemotleft`: is deprecated in favor of `XKB_KEY_guillemetleft`
    - `XKB_KEY_guillemotright`: is deprecated in favor of `XKB_KEY_guillemetright`
    - `XKB_KEY_dead_small_schwa`: is deprecated in favor of `XKB_KEY_dead_schwa`
    - `XKB_KEY_dead_capital_schwa`: is deprecated in favor of `XKB_KEY_dead_SCHWA`

  Relevant upstream merge requests: [xorgproto-83], [xorgproto-84].

[xorgproto-83]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/83
[xorgproto-84]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/84

- Keysyms: Fixed inconsistent results in `xkb_keysym_from_name` when used with
  the flag `XKB_KEYSYM_CASE_INSENSITIVE`. In some rare cases it would return a
  keysym with an upper-case name instead of the expected lower-case (e.g.
  `XKB_KEY_dead_A` instead of `XKB_KEY_dead_a`).

- Keysyms: Fixed case mapping for 3 Latin 1 keysyms:

  - `XKB_KEY_ydiaeresis`
  - `XKB_KEY_mu`
  - `XKB_KEY_ssharp`

- Keysyms: Fixed `xkb_keysym_is_modifier` to detect also the following keysyms:

  - `XKB_KEY_ISO_Level5_Shift`
  - `XKB_KEY_ISO_Level5_Latch`
  - `XKB_KEY_ISO_Level5_Lock`

- Prevent recursive includes of keymap components.

- Fixed global default statements `x.y = z;` in wrong scope not raising an error.

  Contributed by Mikhail Gusarov

- Keymap: Fixed empty symbols declaration.

  Contributed by Yuichiro Hanada.

- Rules: Made newline required after `!include` line.

  Contributed by Mikhail Gusarov.

- Rules: Fixed a bug where variant indexes were ignored with the layout index
  used instead. They are practically always the same, but don't have to be.

  Contributed by @wysiwys.

- Compose: Fixed a segfault with `xkb_compose_table_iterator_next` when used on an
  empty table.

- Compose: Added check to ensure to open only regular files, not e.g. directories.

- Registry: Updated the DTD and always parse the “popularity” attribute.

- Fixed a few memory leaks and keymap symbols parsing.

Tools
-----

### New

- `xkbcli compile-compose`: added new CLI utility to test Compose files.
- `xkbcli interactive-evdev`: added `--verbose` option.
- `xkbcli interactive-x11`: added support for Compose.
- `xkbcli interactive-wayland`: added support for Compose.

### Fixes

- Bash completion: Fixed completion in some corner cases.

Build system
------------

- Fix building with clang when using `-Wl,--gc-sections`.

  Contributed by ppw0.

- Fixed linking using `lld 1.17`.

  Contributed by Baptiste Daroussin.

- Fix building X11 tests on macOS.

- Documentation is no longer built by default; it requires `-Denable-docs=true`.

libxkbcommon 1.6.0 - 2023-10-08
==================

API
---

### Breaking changes

- *Remove* keysyms that were intended for German T3 layout but are unused:

  - `XKB_KEY_dead_lowline`
  - `XKB_KEY_dead_aboveverticalline`
  - `XKB_KEY_dead_belowverticalline`
  - `XKB_KEY_dead_longsolidusoverlay`

  See the upstream [`xorgproto` MR](https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/70). See hereinafter for further changes related to keysyms.

### New

- Add Compose iterator API to iterate the entries in a compose table:

  - `xkb_compose_table_entry_sequence`
  - `xkb_compose_table_entry_keysym`
  - `xkb_compose_table_entry_utf8`
  - `xkb_compose_table_iterator_new`
  - `xkb_compose_table_iterator_free`
  - `xkb_compose_table_iterator_next`

- *Structured log messages* with a message registry. There is an *ongoing* work
  to assign unique identifiers to log messages and add a corresponding error
  index documentation page:

  - The log entries are preceded with an identifier in the form `XKB-NNN`, where
    `NNN` is a decimal number.

  - The log entries can then be parsed with third-party tools, to check for
    specific identifiers.

  - The new documentation page “**Error index**” lists all the kind of error messages
    with their identifiers. The aim is that each entry could present detailed
    information on the error and how to fix it.

- Add a new warning for numeric keysyms references in XKB files: the preferred
  keysym reference form is its name or its Unicode value, if relevant.

- Add the upper bound `XKB_KEYSYM_MAX` to check valid keysyms.

- Add a warning when loading a keymap using RMLVO with no layout but with the
  variant set. The variant is actually discarded and both layout and variant are
  set to default values, but this was done previously with no warning, confusing
  end users.

- Add support for `modifier_map None { … }`. This feature is missing compared to
  the X11 implementation. It allows to reset the modifier map of a key.

- Update keysyms using latest [xorgproto] (commit: `1c8128d72df22843a2022576850bc5ab5e3a46ea`):

  - Additions:

    - `XKB_KEY_ordmasculine` ([xorgproto-68])
    - `XKB_KEY_guillemetleft` ([xorgproto-68])
    - `XKB_KEY_guillemetright` ([xorgproto-68])
    - `XKB_KEY_dead_schwa` ([xorgproto-78])
    - `XKB_KEY_dead_SCHWA` ([xorgproto-78])
    - `XKB_KEY_dead_hamza` ([xorgproto-71])
    - `XKB_KEY_XF86EmojiPicker` ([xorgproto-44])
    - `XKB_KEY_XF86Dictate` ([xorgproto-49])
    - `XKB_KEY_XF86CameraAccessEnable` ([xorgproto-82])
    - `XKB_KEY_XF86CameraAccessDisable` ([xorgproto-82])
    - `XKB_KEY_XF86CameraAccessToggle` ([xorgproto-82])
    - `XKB_KEY_XF86NextElement` ([xorgproto-82])
    - `XKB_KEY_XF86PreviousElement` ([xorgproto-82])
    - `XKB_KEY_XF86AutopilotEngageToggle` ([xorgproto-82])
    - `XKB_KEY_XF86MarkWaypoint` ([xorgproto-82])
    - `XKB_KEY_XF86Sos` ([xorgproto-82])
    - `XKB_KEY_XF86NavChart` ([xorgproto-82])
    - `XKB_KEY_XF86FishingChart` ([xorgproto-82])
    - `XKB_KEY_XF86SingleRangeRadar` ([xorgproto-82])
    - `XKB_KEY_XF86DualRangeRadar` ([xorgproto-82])
    - `XKB_KEY_XF86RadarOverlay` ([xorgproto-82])
    - `XKB_KEY_XF86TraditionalSonar` ([xorgproto-82])
    - `XKB_KEY_XF86ClearvuSonar` ([xorgproto-82])
    - `XKB_KEY_XF86SidevuSonar` ([xorgproto-82])
    - `XKB_KEY_XF86NavInfo` ([xorgproto-82])

  - Deprecations:

    - `XKB_KEY_masculine`: use `XKB_KEY_ordmasculine` instead ([xorgproto-68])
    - `XKB_KEY_guillemotleft`: use `XKB_KEY_guillemetleft` instead ([xorgproto-68])
    - `XKB_KEY_guillemotright`: use `XKB_KEY_guillemetright` instead ([xorgproto-68])
    - `XKB_KEY_dead_small_schwa`: use `XKB_KEY_dead_schwa` instead ([xorgproto-78])
    - `XKB_KEY_dead_capital_schwa`: use `XKB_KEY_dead_SCHWA` instead ([xorgproto-78])

  [xorgproto]: https://gitlab.freedesktop.org/xorg/proto/xorgproto
  [xorgproto-44]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/44
  [xorgproto-49]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/49
  [xorgproto-68]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/68
  [xorgproto-71]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/71
  [xorgproto-78]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/78
  [xorgproto-82]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/82

- Ongoing work to improve the documentation about XKB and its V1 format.

### Fixes

- Prevent `xkb_keysym_from_name` to parse out-of-range hexadecimal keysyms.

- Disallow producing NULL character with escape sequences `\0` and `\x0`.

- Prevent overflow of octal escape sequences by making `\400..\777` invalid.

- Prevent interpreting and emitting invalid Unicode encoding forms: surrogates
  are invalid in both UTF-32 and UTF-8.

- `xkb_keymap_new_from_buffer`: Allow for a NULL-terminated keymap string.

- Compose: Increase the limit of possible entries to handle huge Compose files.

  Contributed by alois31.

Tools
-----

### New

- Add bash completions for `xkbcli` and its subcommands.

- `xkbcli interactive-*`: Add options `--short` to hide some fields.

- `xkbcli interactive-evdev`: Add `--includes` and `--include-defaults` options.

- Add `xkb-check-messages` *experimental* tool (not installed).
  It checks whether given log messages identifiers are supported.

### Fixes

- `xkbcli compile-keymap`: Allow to use without arguments.

- `xkbcli interactive-*`: Always print keycode.

- `xkbcli interactive-*`: Escape control characters for Unicode output, instead of
  printing them as-is, messing the output.

Build system
------------

- Bump required meson to 0.52.0.

- Allow `xkbcommon` to be used as a subproject.

  Contributed by Simon Ser.

- Improve Windows compilation.

libxkbcommon 1.5.0 - 2023-01-02
==================

- Add `xkb_context` flag `XKB_CONTEXT_NO_SECURE_GETENV` and `rxkb_context` flag
  `RXKB_CONTEXT_NO_SECURE_GETENV`.

  xkbcommon uses `getenv_secure()` to obtain environment variables. This flag
  makes xkbcommon use `getenv()` instead.

  This is useful for some clients that have relatively benign capabilities set,
  like CAP_SYS_NICE, that also want to use e.g. the XKB configuration from the
  environment and user configs in XDG_CONFIG_HOME.

  Contributed by Ronan Pigott.

- Fix crash in `xkbcli interactive-wayland` under a compositor which supports
  new versions of the xdg-shell protocol.

  Contributed by Jan Alexander Steffens (heftig).

- Fix some MSVC build issues.

- Fix some issues when including xkbcommon as a meson subproject.

- meson>=0.51 is now required.

- New API:
  XKB_CONTEXT_NO_SECURE_GETENV
  RXKB_CONTEXT_NO_SECURE_GETENV

libxkbcommon 1.4.1 - 2022-05-21
==================

- Fix compose sequence overriding (common prefix) not working correctly.
  Regressed in 1.2.0.

  Contributed by Weng Xuetian.

- Remove various bogus currency sign (particulary Euro and Korean Won) entries
  from the keysym <-> Unicode mappings. They prevented the real
  keysyms/codepoints for these from mapping correctly.

  Contributed by Sam Lantinga and Simon Ser.

libxkbcommon 1.4.0 - 2022-02-04
==================

- Add `enable-tools` option to Meson build (on by default) to allow disabling
  the `xkbcli` tools.

  Contributed by Alex Xu (Hello71).

- In `xkbcli list`, fix "YAML Norway problem" in output.

  Contributed by Peter Hutterer.

- In libxkbregistry, variants now inherit iso639, iso3166 and brief from parent
  layout if omitted.

  Contributed by M Hickford.

- In libxkbregistry, don't call `xmlCleanupParser()` - it's not supposed to
  be called by libraries.

  Contributed by Peter Hutterer.

- In libxkbregistry, skip over invalid ISO-639 or ISO-3166 entries.

  Contributed by Peter Hutterer.

libxkbcommon 1.3.1 - 2021-09-10
==================

- In `xkbcli interactive-x11`, use the Esc keysym instead of the Esc keycode
  for quitting.

  Contributed by Simon Ser.

- In `xkbcli how-to-type`, add `--keysym` argugment for how to type a keysym
  instead of a Unicode codepoint.

- Fix a crash in `xkb_x11_keymap_new_from_device` error handling given some
  invalid keymaps. Regressed in 1.2.0.

  Reported by Zack Weinberg. Tested by Uli Schlachter.

libxkbcommon 1.3.0 - 2021-05-01
==================

- Change `xkbcli list` to output YAML, instead of the previous ad-hoc format.

  This allows to more easily process the information in a programmetic way, for
  example

    xkbcli list | yq -r ".layouts[].layout"

  Contributed by Peter Hutterer.

- Optimize a certain part of keymap compilation (atom interning).

- Fix segmentation fault in case-insensitive `xkb_keysym_from_name` for certain
  values like the empty string.

  Contributed by Isaac Freund.

- Support building libxkbcommon as a meson subproject.

  Contributed by Adrian Perez de Castro.

- Add `ftruncate` fallback for `posix_fallocate` in `xkbcli interactive-wayland`
  for FreeBSD.

  Contributed by Evgeniy Khramtsov.

- Properly export library symbols in MSVC.

  Contributed by Adrian Perez de Castro.

libxkbcommon 1.2.1 - 2021-04-07
==================

- Fix `xkb_x11_keymap_new_from_device()` failing when the keymap contains key
  types with missing level names, like the one used by the `numpad:mac` option
  in xkeyboard-config. Regressed in 1.2.0.

libxkbcommon 1.2.0 - 2021-04-03
==================

- `xkb_x11_keymap_new_from_device()` is much faster. It now performs only 2
  roundtrips to the X server, instead of dozens (in first-time calls).

  Contributed by Uli Schlachter.

- Case-sensitive `xkb_keysym_from_name()` is much faster.

- Keysym names of the form `0x12AB` and `U12AB` are parsed more strictly.
  Previously the hexadecimal part was parsed with `strtoul()`, now only up
  to 8 hexadecimal digits (0-9A-Fa-f) are allowed.

- Compose files now have a size limit (65535 internal nodes). Further sequences
  are discared and a warning is issued.

- Compose table loading (`xkb_compose_table_new_from_locale()` and similar) is
  much faster.

- Use `poll()` instead of `epoll()` for `xlbcli interactive-evdev`, making it
  portable to FreeBSD which provides evdev but not epoll. On FreeBSD, remember
  to install the `evdev-proto` package to get the evdev headers.

- The build now requires a C11 compiler (uses anonymous structs/unions).

libxkbcommon 1.1.0 - 2021-02-27
==================

- Publish the `xkb-format-text-v1.md` file in the HTML documentation. This file
  existed for a long time but only in the Git repository.
  Link: https://xkbcommon.org/doc/current/md_doc_keymap_format_text_v1.html

- Add partial documentation for xkb_symbols to xkb-format-text-v1.md.

  Contributed by Simon Zeni.

- Update keysym definitions to latest xorgproto. In particular, this adds many
  special keysyms corresponding to Linux evdev keycodes.

  Contributed by Peter Hutterer <@who-t.net>.

- New API:
  Too many XKB_KEY_* definitions to list here.

libxkbcommon 1.0.3 - 2020-11-23
==================

- Fix (hopefully) a segfault in xkb_x11_keymap_new_from_device() in some
  unclear situation (bug introduced in 1.0.2).

- Fix keymaps created with xkb_x11_keymap_new_from_device() don't have level
  names (bug introduced in 0.8.0).

libxkbcommon 1.0.2 - 2020-11-20
==================

- Fix a bug where a keysym that cannot be resolved in a keymap gets compiled to
  a garbage keysym. Now it is set to XKB_KEY_NoSymbol instead.

- Improve the speed of xkb_x11_keymap_new_from_device() on repeated calls in the
  same xkb_context().


libxkbcommon 1.0.1 - 2020-09-11
==================

- Fix the tool-option-parsing test failing.

- Remove requirement for pytest in the tool-option-parsing test.

- Make the table output of `xkbcli how-to-type` aligned.

- Some portability and test isolation fixes.

libxkbcommon 1.0.0 - 2020-09-05
==================

Note: this release is API and ABI compatible with previous releases -- the
major version bump is only an indication of stability.

- Add libxkbregistry as configure-time optional library. libxkbregistry is a C
  library that lists available XKB models, layouts and variants for a given
  ruleset. This is a separate library (libxkbregistry.so, pkgconfig file
  xkbregistry.pc) and aimed at tools that provide a listing of available
  keyboard layouts to the user. See the Documentation for details on the API.

  Contributed by Peter Hutterer <@who-t.net>.

- Better support custom user configuration:

    * Allow including XKB files from other paths.

      Previously, a 'symbols/us' file in path A would shadow the same file in
      path B. This is suboptimal, we rarely need to hide the system files - we
      care mostly about *extending* them. By continuing to check other lookup
      paths, we make it possible for a XDG_CONFIG_HOME/xkb/symbols/us file to
      have sections including those from /usr/share/X11/xkb/symbols/us.

      Note that this is not possible for rules files, which need to be manually
      controlled to get the right bits resolved.

    * Add /etc/xkb as extra lookup path for system data files.

      This completes the usual triplet of configuration locations available for
      most processes:
      - vendor-provided data files in /usr/share/X11/xkb
      - system-specific data files in /etc/xkb
      - user-specific data files in $XDG_CONFIG_HOME/xkb

      The default lookup order user, system, vendor, just like everything else
      that uses these conventions.

      For include directives in rules files, the '%E' resolves to that path.

    * Add a new section to the documentation for custom user configuration.

  Contributed by Peter Hutterer <@who-t.net>.

- Add an `xkbcli` command-line utility.

  This tool offers various subcommands for introspection and debugging.
  Currently the available subcommands are:

  list
    List available rules, models, layouts, variants and options

  interactive-wayland
    Interactive debugger for XKB keymaps for Wayland

  interactive-x11
    Interactive debugger for XKB keymaps for X11

  interactive-evdev
    Interactive debugger for XKB keymaps for evdev (Linux)

  compile-keymap
    Compile an XKB keymap

  how-to-type
    See separate entry below.

  See the manpages for usage information.

  Contributed by Peter Hutterer <@who-t.net>.

- Add `xkb_utf32_to_keysym()` to translate a Unicode codepoint to a keysym.
  When a special keysym (`XKB_KEY_` constant) for the codepoint exists, it is
  returned, otherwise the direct encoding is used, if permissible.

  Contributed by Jaroslaw Kubik <@froglogic.com>.

- Add `xkb_keymap_key_get_mods_for_level()` which retrieves sets of modifiers
  which produce a given shift level in a given key+layout.

  Contributed by Jaroslaw Kubik <@froglogic.com>.

- Add `xkbcli how-to-type` command, which, using `xkb_utf32_to_keysym()`
  and `xkb_keymap_key_get_mods_for_level()` and other APIs, prints out all
  the ways to produce a given keysym.

  For example, how to type `?` (codepoint 63) in a us,de keymap?

    $ xkbcli how-to-type --layout us,de 63 | column -ts $'\t'
    keysym: question (0x3f)
    KEYCODE  KEY NAME  LAYOUT#  LAYOUT NAME   LEVEL#  MODIFIERS
    20       AE11      2        German        2       [ Shift ]
    20       AE11      2        German        2       [ Shift Lock ]
    61       AB10      1        English (US)  2       [ Shift ]

- Add a new section to the documentation describing the format of the XKB
  rules file.

- Search for Compose in $XDG_CONFIG_HOME/XCompose (fallback to
  ~/.config/XCompose) before trying $HOME/.XCompose.

  Note that libX11 still only searches in $HOME/.XCompose.

  Contributed by Emmanuel Gil Peyrot <@linkmauve.fr>.

- Bump meson requirement to >= 0.49.0.

- Fix build with byacc.

- Fix building X11 tests on PE targets.

  Contributed by Jon Turney <@dronecode.org.uk>

- The tests no longer rely on bash, only Python (which is already used by
  meson).

- New API:
  xkb_utf32_to_keysym
  xkb_keymap_key_get_mods_for_level
  XKB_KEY_XF86FullScreen


libxkbcommon 0.10.0 - 2020-01-18
===================

- (security) Fix quadratic complexity in the XKB file parser. See commit
  message 7c42945e04a2107827a057245298dedc0475cc88 for details.

- Add $XDG_CONFIG_HOME/xkb to the default search path. If $XDG_CONFIG_HOME
  is not set, $HOME/.config/xkb is used. If $HOME is not set, the path is not
  added.

  The XDG path is looked up before the existing default search path $HOME/.xkb.

  Contributed by Peter Hutterer <@who-t.net>.

- Add support for include statements in XKB rules files.

  This is a step towards making local XKB customizations more tenable and
  convenient, without modifying system files.

  You can now include other rules files like this:

      ! include %S/evdev

  Two directives are supported, %H to $HOME and %S for the system-installed
  rules directory (usually /usr/share/X11/xkb/rules).

  See commit message ca033a29d2ca910fd17b1ae287cb420205bdddc8 and
  doc/rules-format.txt in the xkbcommon source code for more information.

  Contributed by Peter Hutterer <@who-t.net>.

- Downgrade "Symbol added to modifier map for multiple modifiers" log to a
  warning.

  This error message was too annoying to be shown by default. When working on
  keymaps, set `XKB_LOG_LEVEL=debug XKB_LOG_VERBOSITY=10` to see all possible
  messages.

- Support building on Windows using the meson MSVC backend.

  Contributed by Adrian Perez de Castro <@igalia.com>.

- Fix bug where the merge mode only applied to the first vmod in a
  `virtual_modifiers` statement. Given

      augment virtual_modifiers NumLock,Alt,LevelThree

  Previously it was incorrectly treated as

      augment virtual_modifiers NumLock;
      virtual_modifiers Alt;
      virtual_modifiers LevelThree;

  Now it is treated as

      augment virtual_modifiers NumLock;
      augment virtual_modifiers Alt;
      augment virtual_modifiers LevelThree;

- Reject interpret modifier predicate with more than one value. Given

      interpret ISO_Level3_Shift+AnyOf(all,extraneous) { ... };

  Previously, extraneous (and further) was ignored. Now it's rejected.

- Correctly handle capitalization of the ssharp keysym.

- Speed up and improve the internal `xkeyboard-config` tool. This tool
  compiles all layout/variant combinations in the xkeyboard-config dataset
  and reports any issues it finds.

  Contributed by Peter Hutterer <@who-t.net>.

- Speed up "atoms" (string interning). This code goes back at least to X11R1
  (released 1987).


libxkbcommon 0.9.1 - 2019-10-19
==================

- Fix context creation failing when run in privileged processes as defined by
  `secure_getenv(3)`, e.g. GDM.


libxkbcommon 0.9.0 - 2019-10-19
==================

- Move ~/.xkb to before XKB_CONFIG_ROOT (the system XKB path, usually
  /usr/share/X11/xkb) in the default include path. This enables the user
  to have full control of the keymap definitions, instead of only augmenting
  them.

- Remove the Autotools build system. Use the meson build system instead.

- Fix invalid names used for levels above 8 when dumping keymaps. Previously,
  e.g. "Level20" was dumped, but only up to "Level8" is accepted by the
  parser. Now "20" is dumped.

- Change level references to always be dumped as e.g. "5" instead of "Level5".

  Change group references to always be dumped capitalized e.g. "Group3" instead
  of "group3". Previously it was inconsistent.

  These changes affect the output of xkb_keymap_get_as_string().

- Fix several build issues on macOS/Darwin, Solaris, NetBSD, cross compilation.

- Port the interactive-wayland test program to the stable version of xdg-shell.


libxkbcommon 0.8.4 - 2019-02-22
==================

- Fix build of xkbcommon-x11 static library with meson.

- Fix building using meson from the tarball generated by autotools.


libxkbcommon 0.8.3 - 2019-02-08
==================

- Fix build of static libraries with meson.
  (Future note: xkbcommon-x11 was *not* fixed in this release.)

- New API:
  XKB_KEY_XF86MonBrightnessCycle
  XKB_KEY_XF86RotationLockToggle


libxkbcommon 0.8.2 - 2018-08-05
==================

- Fix various problems found with fuzzing (see commit messages for
  more details):

    - Fix a few NULL-dereferences, out-of-bounds access and undefined behavior
      in the XKB text format parser.


libxkbcommon 0.8.1 - 2018-08-03
==================

- Fix various problems found in the meson build (see commit messages for more
  details):

    - Fix compilation on Darwin.

    - Fix compilation of the x11 tests and demos when XCB is installed in a
      non-standard location.

    - Fix xkbcommon-x11.pc missing the Requires specification.

- Fix various problems found with fuzzing and Coverity (see commit messages for
  more details):

    - Fix stack overflow in the XKB text format parser when evaluating boolean
      negation.

    - Fix NULL-dereferences in the XKB text format parser when some unsupported
      tokens appear (the tokens are still parsed for backward compatibility).

    - Fix NULL-dereference in the XKB text format parser when parsing an
      xkb_geometry section.

    - Fix an infinite loop in the Compose text format parser on some inputs.

    - Fix an invalid free() when using multiple keysyms.

- Replace the Unicode characters for the leftanglebracket and rightanglebracket
  keysyms from the deprecated LEFT/RIGHT-POINTING ANGLE BRACKET to
  MATHEMATICAL LEFT/RIGHT ANGLE BRACKET.

- Reject out-of-range Unicode codepoints in xkb_keysym_to_utf8 and
  xkb_keysym_to_utf32.


libxkbcommon 0.8.0 - 2017-12-15
==================

- Added xkb_keysym_to_{upper,lower} to perform case-conversion directly on
  keysyms. This is useful in some odd cases, but working with the Unicode
  representations should be preferred when possible.

- Added Unicode conversion rules for the signifblank and permille keysyms.

- Fixed a bug in the parsing of XKB key type definitions where the number
  of levels were determined by the number of level *names*. Keymaps which
  omit level names were hence miscompiled.

  This regressed in version 0.4.3. Keymaps from xkeyboard-config were not
  affected since they don't omit level names.

- New API:
  xkb_keysym_to_upper()
  xkb_keysym_to_lower()


libxkbcommon 0.7.2 - 2017-08-04
==================

- Added a Meson build system as an alternative to existing autotools build
  system.

  The intent is to remove the autotools build in one of the next releases.
  Please try to convert to it and report any problems.

  See http://mesonbuild.com/Quick-guide.html for basic usage, the
  meson_options.txt for the project-specific configuration options,
  and the PACKAGING file for more details.

  There are some noteworthy differences compared to the autotools build:

  - Feature auto-detection is not performed. By default, all features are
    enabled (currently: docs, x11, wayland). The build fails if any of
    the required dependencies are not available. To disable a feature,
    pass -Denable-<feature>=false to meson.

  - The libraries are either installed as shared or static, as specified
    by the -Ddefault_library=shared/static option. With autotools, both
    versions are installed by default.

  - xorg-util-macros is not used.

  - A parser generator (bison/byacc) is always required - there is no
    fallback to pre-generated output bundled in the tarball, as there is
    in autotools.

- Removed Android.mk support.

- Removed the *-uninstalled.pc pkgconfig files.

- Ported the interactive-wayland demo program to v6 of the xdg-shell
  protocol.

- Added new keysym definitions from xproto.

- New API:
  XKB_KEY_XF86Keyboard
  XKB_KEY_XF86WWAN
  XKB_KEY_XF86RFKill
  XKB_KEY_XF86AudioPreset


libxkbcommon 0.7.1 - 2017-01-18
==================

- Fixed various reported problems when the current locale is tr_TR.UTF-8.

  The function xkb_keysym_from_name() used to perform case-insensitive
  string comparisons in a locale-dependent way, but required it to to
  work as in the C/ASCII locale (the so called "Turkish i problem").

  The function is now no longer affected by the current locale.

- Fixed compilation in NetBSD.


libxkbcommon 0.7.0 - 2016-11-11
==================

- Added support for different "modes" of calculating consumed modifiers.
  The existing mode, based on the XKB standard, has proven to be
  unintuitive in various shortcut implementations.

  A new mode, based on the calculation used by the GTK toolkit, is added.
  This mode is less eager to declare a modifier as consumed.

- Added a new interactive demo program using the Wayland protocol.
  See the PACKAGING file for the new (optional) test dependencies.

- Fixed a compilation error on GNU Hurd.

- New API:
  enum xkb_consumed_mode
  XKB_CONSUMED_MODE_XKB
  XKB_CONSUMED_MODE_GTK
  xkb_state_key_get_consumed_mods2
  xkb_state_mod_index_is_consumed2


libxkbcommon 0.6.1 - 2016-04-08
==================

- Added LICENSE to distributed files in tarball releases.

- Minor typo fix in xkb_keymap_get_as_string() documentation.


libxkbcommon 0.6.0 - 2016-03-16
==================

- If the XKB_CONFIG_ROOT environment variable is set, it is used as the XKB
  configuration root instead of the path determined at build time.

- Tests and benchmarks now build correctly on OSX.

- An XKB keymap provides a name for each key it defines.  Traditionally,
  these names are limited to at most 4 characters, and are thus somewhat
  obscure, but might still be useful (xkbcommon lifts the 4 character limit).

  The new functions xkb_keymap_key_get_name() and xkb_keymap_key_by_name()
  can be used to get the name of a key or find a key by name.  Note that
  a key may have aliases.

- Documentation improvements.

- New API:
  xkb_keymap_key_by_name()
  xkb_keymap_key_get_name()


libxkbcommon 0.5.0 - 2014-10-18
==================

- Added support for Compose/dead keys in a new module (included in
  libxkbcommon). See the documentation or the
  xkbcommon/xkbcommon-compose.h header file for more details.

- Improved and reordered some sections of the documentation.

- The doxygen HTML pages were made nicer to read.

- Most tests now run also on non-linux platforms.

- A warning is emitted by default about RMLVO values which are not used
  during keymap compilation, which are most often a user misconfiguration.
  For example, "terminate:ctrl_alt_backspace" instead of
  "terminate:ctrl_alt_bksp".

- Added symbol versioning for libxkbcommon and libxkbcommon-x11.
  Note: binaries compiled against this and future versions will not be
  able to link against the previous versions of the library.

- Removed several compatablity symbols from the binary (the API isn't
  affected). This affects binaries which

  1. Were compiled against a pre-stable (<0.2.0) version of libxkbcommon, and
  2. Are linked against the this or later version of libxkbcommon.

  Such a scenario is likely to fail already.

- If Xvfb is not available, the x11comp test is now correctly skipped
  instead of hanging.

- Benchmarks were moved to a separate bench/ directory.

- Build fixes from OpenBSD.

- Fixed a bug where key type entries such as "map[None] = Level2;" were
  ignored.

- New API:
  XKB_COMPOSE_*
  xkb_compose_*


libxkbcommon 0.4.3 - 2014-08-19
==================

- Fixed a bug which caused xkb_x11_keymap_new_from_device() to misrepresent
  modifiers for some keymaps.

  https://github.com/xkbcommon/libxkbcommon/issues/9

- Fixed a bug which caused xkb_x11_keymap_new_from_device() to ignore XKB
  PrivateAction's.

- Modifiers are now always fully resolved after xkb_state_update_mask().
  Previously the given state components were used as-is, without
  considering virtual modifier mappings.
  Note: this only affects non-standard uses of xkb_state_update_mask().

- Added a test for xkbcommon-x11, "x11comp". The test uses the system's
  Xvfb server and xkbcomp. If they do not exist or fail, the test is
  skipped.

- Fixed memory leaks after parse errors in the XKB yacc parser.
  The fix required changes which are currently incompatible with byacc.


libxkbcommon 0.4.2 - 2014-05-15
==================

- Fixed a bug where explicitly passing "--enable-x11" to ./configure would
  in fact disable it (regressed in 0.4.1).

- Added @since version annotations to the API documentation for everything
  introduced after the initial stable release (0.2.0).

- Added a section to the documentation about keysym transformations, and
  clarified which functions perform a given transformation.

- XKB files which fail to compile during keymap construction can no longer
  have any effect on the resulting keymap: changes are only applied when
  the entire compilation succeeds.
  Note: this was a minor correctness issue inherited from xkbcomp.

- Fix an out-of-bounds array access in src/x11/util.c:adopt_atoms()
  error-handling code.
  Note: it seems impossible to trigger in the current code since the input
  size cannot exceed the required size.


libxkbcommon 0.4.1 - 2014-03-27
==================

- Converted README to markdown and added a Quick Guide to the
  documentation, which breezes through the most common parts of
  xkbcommon.

- Added two new functions, xkb_state_key_get_utf{8,32}(). They
  combine the operations of xkb_state_key_get_syms() and
  xkb_keysym_to_utf{8,32}(), and provide a nicer interface for it
  (espcially for multiple-keysyms-per-level).

- The xkb_state_key_get_utf{8,32}() functions now apply Control
  transformation: when the Control modifier is active, the string
  is converted to an appropriate control character.
  This matches the behavior of libX11's XLookupString(3), and
  required by the XKB specification:
  https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Control_Modifier

  https://bugs.freedesktop.org/show_bug.cgi?id=75892

- The consumed modifiers for a key are now calculated similarly
  to libX11. The previous behavior caused a bug where Shift would
  not cancel an active Caps Lock.

- Make xkbcommon-x11 work with the keymap reported by the XQuartz
  X server.

  https://bugs.freedesktop.org/show_bug.cgi?id=75798

- Reduce memory usage during keymap compilation some more.

- New API:
  xkb_state_key_get_consumed_mods()
  xkb_state_key_get_utf8()
  xkb_state_key_get_utf32()

- Deprecated API:
  XKB_MAP_COMPILE_PLACEHOLDER, XKB_MAP_NO_FLAGS
    use XKB_KEYMAP_NO_FLAGS instead.

- Bug fixes.


libxkbcommon 0.4.0 - 2014-02-02
==================

- Add a new add-on library, xkbcommon-x11, to support creating keymaps
  with the XKB X11 protocol, by querying the X server directly.
  See the xkbcommon/xkbcommon-x11.h header file for more details.
  This library requires libxcb-xkb >= 1.10, and is enabled by default.
  It can be disabled with the --disable-x11 configure switch.
  Distributions are encouraged to split the necessary files for this
  library (libxkbcommon-x11.so, xkbcommon-x11.pc, xkbcommon/xkbcommon-x11.h)
  to a separate package, such that the main package does not depend on
  X11 libraries.

- Fix the keysym <-> name lookup table to not require huge amounts of
  relocations.

- Fix a bug in the keysym <-> name lookup, whereby lookup might fail in
  some rare cases.

- Reduce memory usage during keymap compilation.

- New API:
  New keysyms from xproto 7.0.25 (German T3 layout keysyms).
  XKB_MOD_NAME_NUM for the usual NumLock modifier.
  xkb_x11_* types and functions, XKB_X11_* constants.


libxkbcommon 0.3.2 - 2013-11-22
==================

- Log messages from the library now look like "xkbcommon: ERROR" by
  default, instead of xkbcomp-like "Error:   ".

- Apply capitalization transformation on keysyms in
  xkb_keysym_get_one_sym(), to match the behavior specified in the XKB
  specification:
  https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Lock_Modifier

- Support byacc for generating the parser, in addition to Bison.

- New API:
  XKB_KEY_XF86AudioMicMute keysym from xproto 7.0.24.
  XKB_KEYSYM_NO_FLAGS
  XKB_CONTEXT_NO_FLAGS
  XKB_MAP_COMPILE_NO_FLAGS

- Bug fixes.


libxkbcommon 0.3.1 - 2013-06-03
==================

- Replace the flex scanner with a hand-written one. flex is no longer
  a build requirement.

- New API:
  xkb_keymap_min_keycode()
  xkb_keymap_max_keycode()
  xkb_keymap_key_for_each()


libxkbcommon 0.3.0 - 2013-04-01
==================

- Allow passing NULL to *_unref() functions; do nothing instead of
  crashing.

- The functions xkb_keymap_num_levels_for_key() and
  xkb_keymap_get_syms_by_level() now allow out-of-range values for the
  'layout' parameter. The functions now wrap the value around the number
  of layouts instead of failing.

- The function xkb_keysym_get_name() now types unicode keysyms in
  uppercase and 0-padding, to match the format used by XKeysymToString().

- Building Linux-specific tests is no longer attempted on non-Linux
  environments.

- The function xkb_keymap_new_from_names() now accepts a NULL value for
  the 'names' parameter, instead of failing. This is equivalent to passing
  a 'struct xkb_rule_names' with all fields set to NULL.

- New API:
  xkb_keymap_new_from_buffer()

- Bug fixes.
