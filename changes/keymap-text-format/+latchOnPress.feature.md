Added the new parameter `latchOnPress` for the key action `LatchMods()`.

Some keyboard layouts use `ISO_Level3_Latch` or `ISO_Level5_Latch` to define
“built-in” dead keys. `latchOnPress` enables to behave as usual dead keys, i.e.
to latch on press and to deactivate as soon as another (non-modifier) key is
pressed.

As it is incompatible with X11, this feature is available only using the keymap
text format v2.

[XKB protocol key actions]: https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Key_Actions
