The tools now use:
- `XKB_KEYMAP_FORMAT_TEXT_V2` as a default *input* format.
- `XKB_KEYMAP_USE_ORIGINAL_FORMAT` as a default *output* format.
  So if the input format is not specified, it will resolve to
  `XKB_KEYMAP_FORMAT_TEXT_V2`.
