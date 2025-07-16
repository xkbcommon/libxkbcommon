Virtual modifiers are now mapped to their *canonical encoding* if they are
not mapped *explicitly* (`virtual_modifiers M = …`) nor *implicitly*
(using `modifier_map`/`virtualModifier`).

This feature is enabled only when using `XKB_KEYMAP_FORMAT_TEXT_V2`, as it may
result in encodings not compatible with X11.
