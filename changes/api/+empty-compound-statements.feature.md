Added support for further empty compound statements:
- `xkb_types`: `type \"xxx\" {};`
- `xkb_compat`: `interpret x {};` and `indicator \"xxx\" {};`.

Such statements are initialized using the current defaults, i.e. the
factory implicit defaults or some explicit custom defaults (e.g.
`indicator.modifiers = Shift`).
