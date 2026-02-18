#include "config.h"

#include "xkbcommon/xkbcommon.h" // IWYU pragma: keep
#include "xkbcommon/xkbcommon-compose.h" // IWYU pragma: keep
#include "xkbcommon/xkbcommon-features.h" // IWYU pragma: keep

#ifdef HAS_XKB_X11
#include "xkbcommon/xkbcommon-x11.h" // IWYU pragma: keep
#endif

#ifdef HAS_XKB_REGISTRY
#include "xkbcommon/xkbregistry.h" // IWYU pragma: keep
#endif

auto main() -> int {}
