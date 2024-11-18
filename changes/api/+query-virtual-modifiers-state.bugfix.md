The following functions now allow to query also *virtual* modifiers, so they work
with *any* modifiers (real *and* virtual):
- `xkb_state_mod_index_is_active`
- `xkb_state_mod_indices_are_active`
- `xkb_state_mod_name_is_active`
- `xkb_state_mod_names_are_active`
- `xkb_state_mod_index_is_consumed`
- `xkb_state_mod_index_is_consumed2`
- `xkb_state_mod_mask_remove_consumed`

Warning: they may overmatch in case there are overlappings virtual-to-real
modifiers mappings.
