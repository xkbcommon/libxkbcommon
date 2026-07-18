Extended the enumeration `xkb_keymap_serialize_flags` to force some values to be explicit:
- `::XKB_KEYMAP_SERIALIZE_EXPLICIT_DEFAULT_VALUES`
- `::XKB_KEYMAP_SERIALIZE_EXPLICIT_VMODS`
- `::XKB_KEYMAP_SERIALIZE_EXPLICIT_KEY_VALUES`

This is useful mainly for debugging.
