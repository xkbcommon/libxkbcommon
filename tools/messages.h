/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#ifndef CHECK_MESSAGES_H
#define CHECK_MESSAGES_H

#include "messages-codes.h"

struct xkb_message_entry {
    const xkb_message_code_t code;
    const char *label;
};

int
xkb_message_get_all(const struct xkb_message_entry **xkb_messages);

const struct xkb_message_entry*
xkb_message_get(xkb_message_code_t code);

#endif
