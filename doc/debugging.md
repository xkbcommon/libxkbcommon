# Debugging {#debugging}

## Available tools

<!--! @rawHtml -->
xkbcommon provides multiple <abbr title="Command-Line Interface">CLI</abbr>
tools for debugging, grouped under the <code>xkbcli</code> application.
Please consult the manual pages `man xkbcli` for the complete documentation
of each tool.
<!--! @endRawHtml -->

@note These tools may not be installed by default; please ensure you have the
`libxkbcommon-tools` package installed.

<dl>
<dt>`xkbcli interactive`</dt>
<dd>
Test your configuration interactively. It chooses the appropriate backend based
on the session type. Alternatively you may select explicitly the backend:

<dl>
<dt>`xkbcli interactive-wayland`</dt>
<dd>Test in a *Wayland* session.</dd>
<dt>`xkbcli interactive-x11`</dt>
<dd>Test in an *X11* session.</dd>
<dt>`xkbcli interactive-evdev`</dt>
<dd>
Test raw input events directly.
This requires access to the `/dev/input/event*` devices, you may need to add your
user to the `input` group or run as root.
</dd>
</dl>

> [!TIP]
> You may want to use the flag `--enable-compose` if your layouts use dead keys.
</dd>

<dt>`xkbcli dump-keymap`</dt>
<dd>
Dump an XKB keymap from a display server. It chooses the appropriate backend based
on the session type. Alternatively you may select explicitly the backend:

<dl>
<dt>`xkbcli dump-keymap-wayland`</dt>
<dd>Dump an XKB keymap from a *Wayland* compositor.</dd>
<dt>`xkbcli dump-keymap-x11`</dt>
<dd>Dump an XKB keymap from an *X11* server.</dd>
</dl>

</dd>

<dt>`xkbcli how-to-type`</dt>
<dd>
Find the required key combinations to produce a specific character or keysym.
</dd>

<dt>`xkbcli list`</dt>
<dd>List available layouts, variants, and options provided by an XKB database.</dd>

<dt>`xkbcli compile-keymap`</dt>
<dd>
Compile a keymap and inspect its properties.

> [!TIP]
> Use the options `--explicit-*` to force the corresponding values to be explicit.
> This is especially useful to debug [compatibility interpretations].
</dd>

<dt>`xkbcli compile-compose`</dt>
<dd>
Compile [Compose](@ref compose) files.</dd>

<dt>`xkbcli info`</dt>
<dd>Print information about xkbcommon configuration.</dd>
</dl>

[compatibility interpretations]: @ref interpret-statements

## Error index {#debugging-error-index}

Each error has a unique identifier printed as `[XKB-nnn]` in the log.

See the [error index](@ref error-index) for the documentation of each error.

## Testing a custom configuration {#testing-custom-config}

@note An erroneous XKB configuration may make your keyboard unusable. Therefore
it is advised to try custom configurations safely with the following workflow:

1. Create a directory for the custom configuration, e.g. `~/xkb-test`. Note that
   in order to test it safely, it should *not* be one of the locations that
   [xkbcommon searches][user-configuration locations].
2. Create the relevant sub-directories and files, e.g. `~/xkb-test/symbols/my-layout`.
3. Test if your changes *compile* successfully:

   ```bash
   xkbcli compile-keymap --include ~/xkb-test --include-defaults --test --layout my-layout
   ```

   @note The order of the `--include*` arguments is important here.

   If it does not compile, you may add the flag `--verbose` for additional information.
4. Test if it *behaves* correctly. Note that you may need to add your user to the
   `input` group or run as root.

   ```bash
   xkbcli interactive-evdev --include ~/xkb-test --include-defaults --enable-compose --layout my-layout
   ```
5. Repeat steps 3 and 4 with your *full* keyboard configuration, i.e. all your
   model, layouts and options.
6. If everything works as expected, it is time to test in real conditions:
   - Wayland: , move `~/xkb-test` to one of the [user-configuration locations],
     restart your session, update the keyboard configuration using your usual UI
     and enjoy your XKB customization!
   - X11: Unfortunately Xorg does not support alternative paths, so the next best
     option is to use the venerable `xkbcomp` tool.

     ```bash
     xkbcli compile-keymap --include ~/xkb-test --include-defaults --layout my-layout \
       | xkbcomp - $DISPLAY
    ```

Happy hacking!

[user-configuration locations]: @ref xkb-data-locations
