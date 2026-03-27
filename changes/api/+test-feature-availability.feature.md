Added `xkb_feature_supported()`, `enum xkb_feature` and the corresponding header
`xkbcommon-features.h`. They enable to test feature availability, which is
useful when the library is dynamically linked.
Currently they support only testing enumerations and their values.
