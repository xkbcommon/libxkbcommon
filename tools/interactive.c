/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "src/utils.h"
#include "tools-common.h"

#ifdef KEYMAP_DUMP
#define TOOL "dump-keymap"
#else
#define TOOL "interactive"
#endif

int
main(int argc, char **argv)
{
    const char *new_argv[64] = {NULL};

    new_argv[0] = select_backend(
#if HAVE_XKBCLI_INTERACTIVE_WAYLAND
        TOOL "-wayland",
#else
        NULL,
#endif
#if HAVE_XKBCLI_INTERACTIVE_X11
        TOOL "-x11",
#else
        NULL,
#endif
#if HAVE_XKBCLI_INTERACTIVE_EVDEV && !defined(KEYMAP_DUMP)
        TOOL "-evdev"
#else
        NULL
#endif
    );

    if (new_argv[0] == NULL) {
        fprintf(
            stderr,
            "ERROR: Unable to find a proper backend for "
#ifdef KEYMAP_DUMP
            "keymap dumping\n"
#else
            "interactive debugging\n"
#endif
        );
        return EXIT_FAILURE;
    }

    for (int k = 1; k < MIN(argc, (int) ARRAY_SIZE(new_argv)); k++) {
        new_argv[k] = argv[k];
    }

    return tools_exec_command("xkbcli", argc, new_argv);
}
