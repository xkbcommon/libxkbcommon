context: The default include paths initialization is delayed until required.

This is more efficient for clients that only get the keymap from the server and
thus do not need to look up any XKB files.

It also fixes the issue of containerized apps that lack access to XKB directories.

This is marked as a breaking change, because changing environment variables
relevant to include paths between the call to `xkb_context_new()` and any
function requiring the default path initialization — e.g.
`xkb_keymap_new_from_names()` — will have a different behavior than previous
xkbcommon versions. However this situation is deemed unlikely.
