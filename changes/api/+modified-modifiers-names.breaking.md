The following constants for modifiers names were modified to match the actual
names and not their corresponding real modifier mapping:
- `XKB_MOD_NAME_ALT`: `Alt` (was: `Mod1`)
- `XKB_MOD_NAME_LOGO`: `Super` (was: `Mod4`)
- `XKB_MOD_NAME_NUM`: `NumLock` (was: `Mod2`)

This is a breaking change per se, but together with the changes in the functions
`xkb_state_mod_*_active` it should has no visible impact. In fact it will make
these functions resilient to unusual modifiers mappings.
