# Debugging {#debugging}

## Available tools

xkbcommon provides multiple tools for debugging. Please consult the manual pages
`man xkbcli` for the complete documentation of each tool.

@note These tools may not be installed by default; please ensure you have the
`libxkbcommon-tools` package installed.

### Basic tools

- Interactive:
  - `xkbcli interactive` to test your current configuration by detecting the
    session type.
  - `xkbcli interactive-x11` to test your current configuration in an *X11* session.
  - `xkbcli interactive-wayland` to test your current configuration in a *Wayland*
    session.
  You may want to use the flag `--enable-compose` if your layouts use dead keys.
- `xkbcli how-to-type`: to find the key combinations to type in order to get a
  specific character or keysym.

### Advanced tools

- `xkbcli interactive-evdev`: to test a configuration without affecting your
  current configuration. This requires access to the `/dev/input/event*` devices,
  you may need to add your user to the `input` group or run as root.
- `xkbcli compile-keymap`: to check the resulting compiled keymap for some
  configuration. Use the option `--explicit-values` to force all values to be
  explicit. This is especially useful to debug [compatibility interpretations].
- `xkbcli compile-compose`: to check the resulting Compose file.

[compatibility interpretations]: @ref interpret-statements

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

[user-configuration locations]: @ref user-config-locations
