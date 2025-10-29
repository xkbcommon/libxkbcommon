# Packaging keyboard layouts {#packaging-keyboard-layouts}

Since version **1.13** xkbcommon facilitates installing keyboard layout packages
using a new mechanism: the XKB **extensions directories**. It provides a proper
way for keyboard layout packages to install their files, which otherwise resorted
to hacking the xkeyboard-config system directory.

@note For *single-user* setups, see also @ref user-configuration "".

@tableofcontents{html:2}

## The extensions directories

There are 2 root **extensions directories**, both optional:

<dl>
<dt>
<em>Unversioned</em> directory:
<code>XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH</code>
</dt>
<dd>
It is intended as the *default* extensions path and for *forward* compatibility.

It defaults to: `<xkeyboard-config-root>` - version suffix + `.d`, e.g.
`/usr/share/xkeyboard-config.d`.
</dd>
<dt>
<em>Versioned</em> directory:
<code>XKB_CONFIG_VERSIONED_EXTENSIONS_PATH</code>
</dt>
<dd>
It is intended for *backward compatibility* with older xkeyboard-config packages.

It defaults to: `<xkeyboard-config-root>` + `.d`,
e.g. `/usr/share/xkeyboard-config-2.d`.
</dd>
</dl>

@note `XKB_CONFIG_VERSIONED_EXTENSIONS_PATH` has higher priority than
`XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH`.

## Lookup mechanism

When appending the default include paths, after the *extra* path but before
the *canonical XKB root*, for each extensions directory hereinabove: its
subdirectories are appended in *lexicographic order* to the include paths;
*unversioned* subdirectories are appended only if there is no corresponding
*versioned* subdirectory.

Example: given the following setup:
- `XKEYBOARD_CONFIG_ROOT` = `/usr/share/xkeyboard-config-2`
- `/usr/share/xkeyboard-config-2.d` tree:
  - `p1` (installed by package *p1*)
    - `rules/evdev.xml`
    - `symbols/a`
  - `p2` (installed by package *p2*)
    - `rules/evdev.xml`
    - `symbols/b`
- `/usr/share/xkeyboard-config.d` tree:
  - `p1` (installed by package *p1*)
    - `rules/evdev.xml`
    - `symbols/a`
  - `p3` (installed by package *p3*)
    - `rules/evdev.xml`
    - `symbols/c`

then the include path list may be:
1. `~/.config/xkb` (*user* path)
2. `~/.xkb` (alternative *user* path)
3. `/etc/xkb` (*extra* path)
4. `/usr/share/xkeyboard-config-2.d/p1`
5. `/usr/share/xkeyboard-config-2.d/p2`
6. `/usr/share/xkeyboard-config.d/p3`
7. `/usr/share/xkeyboard-config-2` (*canonical root*)

In this example:
- Package *p1* distributes a layout with specific compatibility files for
  xkeyboard-config v2 and fallback files for v3+. In this case the
  versioned files have priority, so the unversioned
  `/usr/share/xkeyboard-config.d/p1` path is not included.
- Package *p2* distributes only a layout specific to xkeyboard-config v2.
- Package *p3* distributes a layout for any xkeyboard-config version.

## Instructions for keyboard layout packagers

1. Add a dependency to `libxkbcommon` 1.13+ and `xkeyboard-config` 2.45+ using
   their pkg-config files. The `libxkbcommon` dependency will be necessary until
   xkeyboard-config ships a pkg-config file with the needed variables (see
   hereinafter).
2. Remove any installation step that modifies the `xkeyboard-config` directories,
   unless there is a need to provide the layout for *X11-only* sessions, because
   they do not support XKB directories other than the hard-coded one in the X
   server.
3. Install the XKB files in:
   `<XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH>/<PACKAGE_NAME>/<KCCGST_DIR>`, where:
   <dl>
   <dt><code>XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH</code></dt>
   <dd>
   Set from the variable `xkb_unversioned_extensions_path` from the libxkbcommon
   or xkeyboard-config pkg-config file
   </dd>
   <dt><code>PACKAGE_NAME</code></dt>
   <dd>A *unique* identifier, typically the layout or package name.</dd>
   <dt><code>KCCGST_DIR</code></dt>
   <dd>A XKB [KcCGST] component directory, such as `symbols`, `types`, etc.</dd>
   </dl>

   @note If a custom *rules* file is required (e.g. to add an *option*), name it
   `<ruleset>.post` (note the `.post` suffix) to enable composability with other
   packages. See @ref rmlvo-resolution "" for further details.
4. Install the *registry file* at:
   `<XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH>/<PACKAGE_NAME>/rules/evdev.xml`.

   See @ref discoverable-layouts "" for further information.
5. When xkeyboard-config v3 is out:

   - *If* the keymap is updated to use features incompatible with xkeyboard-config v2,
     the v2 files are installed in `<XKB_CONFIG_VERSIONED_EXTENSIONS_PATH_V2>`
     (previously `<XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH>`) and then install the v3
     files with the new features in `<XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH>`.
   - *Otherwise* no change is required.

6. When xkeyboard-config v4+ is out, proceed similarly to step 5 with the
   corresponding xkeyboard-config version.

[KcCGST]: @ref KcCGST-intro
