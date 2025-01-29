/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "test.h"
#include "messages-codes.h"
#include "messages.h"

static void
test_message_get(void)
{
    const struct xkb_message_entry* entry;
    /* Invalid codes */
    /* NOTE: 0 must not be a valid message code */
    entry = xkb_message_get(0);
    assert(entry == NULL);
    entry = xkb_message_get(_XKB_LOG_MESSAGE_MIN_CODE - 1);
    assert(entry == NULL);
    entry = xkb_message_get(_XKB_LOG_MESSAGE_MAX_CODE + 1);
    assert(entry == NULL);

    /* Valid codes */
    entry = xkb_message_get(_XKB_LOG_MESSAGE_MIN_CODE);
    assert(entry != NULL);
    entry = xkb_message_get(XKB_WARNING_CANNOT_INFER_KEY_TYPE);
    assert(entry != NULL);
    entry = xkb_message_get(XKB_ERROR_INVALID_SYNTAX);
    assert(entry != NULL);
    entry = xkb_message_get(XKB_WARNING_CONFLICTING_KEY_FIELDS);
    assert(entry != NULL);
    entry = xkb_message_get(_XKB_LOG_MESSAGE_MAX_CODE);
    assert(entry != NULL);
}

int
main(void)
{
    test_init();

    test_message_get();
}
