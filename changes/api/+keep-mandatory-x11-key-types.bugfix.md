X11: Added a fix to circumvent libX11 and xserver improperly handling key types
in the following cases:
- missing XKB mandatory key types;
- missing level names in key types.
The fix prevents triggering an error when retrieving such keymap using libxkbcommon-x11.
