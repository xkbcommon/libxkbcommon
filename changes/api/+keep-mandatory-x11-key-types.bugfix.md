X11: Added a fix to circumvent libX11 and xserver improperly handling key types
when missing XKB canonical key types.
The fix prevents triggering an error when retrieving such keymap using libxkbcommon-x11.
