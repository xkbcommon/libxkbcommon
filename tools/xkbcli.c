/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "tools-common.h"

static void
usage(void)
{
    printf("Usage: xkbcli [--help|-h] [--version|-V] <command> [<args>]\n"
           "\n"
           "Global options:\n"
           "  -h, --help ...... show this help and exit\n"
           "  -V, --version ... show version information and exit\n"
           /* WARNING: The following is parsed by the bash completion script.
            *          Any change to the format (in particular to the indentation)
            *          should kept in the script in sync. */
           "Commands:\n"
#if HAVE_XKBCLI_LIST
           "  list\n"
           "    List available rules, models, layouts, variants and options\n"
           "\n"
#endif
#if HAVE_XKBCLI_INTERACTIVE_WAYLAND || HAVE_XKBCLI_INTERACTIVE_X11
           "  interactive\n"
           "    Interactive debugger for XKB keymaps; automatically select from"
           "    the following backends, if available: Wayland, X11 and evdev.\n"
           "\n"
#endif
#if HAVE_XKBCLI_INTERACTIVE_WAYLAND
           "  interactive-wayland\n"
           "    Interactive debugger for XKB keymaps for Wayland\n"
           "\n"
#endif
#if HAVE_XKBCLI_INTERACTIVE_X11
           "  interactive-x11\n"
           "    Interactive debugger for XKB keymaps for X11\n"
           "\n"
#endif
#if HAVE_XKBCLI_INTERACTIVE_EVDEV
           "  interactive-evdev\n"
           "    Interactive debugger for XKB keymaps for evdev\n"
           "\n"
#endif
#if HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND || HAVE_XKBCLI_DUMP_KEYMAP_X11
           "  dump-keymap\n"
           "    Dump a XKB keymap from a Wayland or X11 compositor\n"
           "\n"
#endif
#if HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND
           "  dump-keymap-wayland\n"
           "    Dump a XKB keymap from a Wayland compositor\n"
           "\n"
#endif
#if HAVE_XKBCLI_DUMP_KEYMAP_X11
           "  dump-keymap-x11\n"
           "    Dump a XKB keymap from an X server\n"
           "\n"
#endif
#if HAVE_XKBCLI_COMPILE_KEYMAP
           "  compile-keymap\n"
           "    Compile an XKB keymap\n"
           "\n"
#endif
#if HAVE_XKBCLI_COMPILE_COMPOSE
           "  compile-compose\n"
           "    Compile a Compose file\n"
           "\n"
#endif
#if HAVE_XKBCLI_HOW_TO_TYPE
           "  how-to-type\n"
           "    Print key sequences to type a Unicode codepoint\n"
           "\n"
#endif
           );
}

int
main(int argc, char **argv)
{
    enum options {
        OPT_HELP = 1,
        OPT_VERSION,
    };
    int option_index = 0;

    setlocale(LC_ALL, "");

    while (1) {
        int c;
        static struct option opts[] = {
            { "help",    no_argument, 0, OPT_HELP },
            { "version", no_argument, 0, OPT_VERSION },
            { 0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "+hV", opts, &option_index);
        if (c == -1)
            break;

        switch(c) {
            case 'h':
            case OPT_HELP:
                usage();
                return EXIT_SUCCESS;
            case 'V':
            case OPT_VERSION:
                printf("%s\n", LIBXKBCOMMON_VERSION);
                return EXIT_SUCCESS;
            default:
                usage();
                return EXIT_INVALID_USAGE;
        }
    }

    if (optind >= argc) {
        usage();
        return EXIT_INVALID_USAGE;
    }

    argv += optind;
    argc -= optind;

    return tools_exec_command("xkbcli", argc, (const char **) argv);
}
