/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messages-codes.h"
#include "messages.h"

static enum xkb_message_code
parse_message_code(char *raw_code) {
    const enum xkb_message_code code = atoi(raw_code);
    if (!code && strstr(raw_code, "XKB-")) {
        return atoi(&(raw_code[4]));
    } else {
        return code;
    }
}

static void
usage(char **argv)
{
    printf("Usage: %s MESSAGE_CODES\n"
           "\n"
           "Check whether the given message codes are supported.\n",
           argv[0]);

    const struct xkb_message_entry *xkb_messages;
    size_t count = xkb_message_get_all(&xkb_messages);

    printf("\nCurrent supported messages:\n");
    for (size_t idx = 0; idx < count; idx++) {
        printf("- XKB-%03u: %s\n", xkb_messages[idx].code, xkb_messages[idx].label);
    }
}

#define XKB_CHECK_MSG_ERROR_PREFIX "xkb-check-messages: ERROR: "
#define MALFORMED_MESSAGE   (1 << 2)
#define UNSUPPORTED_MESSAGE (1 << 3)

int main(int argc, char **argv) {

    setlocale(LC_ALL, "");

    if (argc <= 1) {
        usage(argv);
        return EXIT_INVALID_USAGE;
    }

    int rc = 0;
    enum xkb_message_code code;
    const struct xkb_message_entry* entry;
    for (int k=1; k < argc; k++) {
        code = parse_message_code(argv[k]);
        if (!code) {
            fprintf(stderr,
                    XKB_CHECK_MSG_ERROR_PREFIX "Malformed message code: %s\n",
                    argv[k]);
            rc |= MALFORMED_MESSAGE;
            continue;
        }
        entry = xkb_message_get(code);
        if (entry == NULL) {
            fprintf(stderr,
                    XKB_CHECK_MSG_ERROR_PREFIX "Unsupported message code: %s\n",
                    argv[k]);
            rc |= UNSUPPORTED_MESSAGE;
        }
    }

    return rc;
}
