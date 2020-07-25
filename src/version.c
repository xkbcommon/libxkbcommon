#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "utils.h"

XKB_EXPORT struct xkb_library_version
xkb_get_library_version() {
    struct xkb_library_version version = {
        .major = LIBXKBCOMMON_VERSION_MAJOR,
        .minor = LIBXKBCOMMON_VERSION_MINOR,
        .patch = LIBXKBCOMMON_VERSION_PATCH,
    };
    return version;
}
