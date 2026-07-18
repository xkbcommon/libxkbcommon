/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"
#include "abi-check.h"
#include "context.h"

void
xkb_log_abi_error(struct xkb_context * restrict ctx,
                  const char * restrict func, enum xkb_error_code error)
{
    switch (error) {
    case XKB_ERROR_ABI_INVALID_STRUCT_SIZE:
        log_err(ctx, XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
                "%s: ABI error: unsupported versioned struct\n", func);
        break;
    case XKB_ERROR_ABI_BACKWARD_COMPAT:
        log_err(ctx, XKB_ERROR_ABI_BACKWARD_COMPAT,
                "%s: ABI version mismatch: missing newer required fields\n",
                func);
        break;
    case XKB_ERROR_ABI_FORWARD_COMPAT:
        log_err(ctx, XKB_ERROR_ABI_FORWARD_COMPAT,
                "%s: ABI version mismatch: cannot use newer fields\n", func);
        break;
    default:
        /* unreachable */
        assert(false);
    }
}
