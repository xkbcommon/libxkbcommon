All keymap components are now optional, e.g. a keymap without a `xkb_types`
section is now legal. The missing components will still be serialized explicitly
in order to maintain backward compatibility.
