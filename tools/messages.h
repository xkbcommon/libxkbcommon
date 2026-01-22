/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include "messages-codes.h"

struct xkb_message_entry {
    enum xkb_message_code code;
    const char *label;
};

int
xkb_message_get_all(const struct xkb_message_entry **xkb_messages);

const struct xkb_message_entry*
xkb_message_get(enum xkb_message_code code);
