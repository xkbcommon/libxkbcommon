The *default* XKB root directory is now set from the *most recent*
[xkeyboard-config](https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config)
installed package, in case [multiple versions](https://xkeyboard-config.freedesktop.org/doc/versioning/)
are installed in parallel.
If no such package is found, it fallbacks to the historical X11 directory, as previously.
