/*
 * Copyright © 2021 Ran Benita <ran@unusedvar.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "src/compose/dump.h"

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [--help] [--file FILE] [--locale LOCALE]\n",
            progname);
    fprintf(fp,
            "\n"
            "Compile a Compose file and print it\n"
            "\n"
            "Options:\n"
            " --help\n"
            "    Print this help and exit\n"
            " --file FILE\n"
            "    Specify a Compose file to load\n"
            " --locale LOCALE\n"
            "    Specify the locale directly, instead of relying on the environment variables\n"
            "    LC_ALL, LC_TYPE and LANG.\n");
}

static bool
print_compose_table_entry(struct xkb_compose_table_entry *entry)
{
    size_t nsyms;
    const xkb_keysym_t *syms = xkb_compose_table_entry_sequence(entry, &nsyms);
    char buf[128];
    for (size_t i = 0; i < nsyms; i++) {
        xkb_keysym_get_name(syms[i], buf, sizeof(buf));
        printf("<%s>", buf);
        if (i + 1 < nsyms) {
            printf(" ");
        }
    }
    printf(" : ");
    const char *utf8 = xkb_compose_table_entry_utf8(entry);
    if (*utf8 != '\0') {
        char *escaped = escape_utf8_string_literal(utf8);
        if (!escaped) {
            fprintf(stderr, "ERROR: Cannot escape the string: allocation error\n");
            return false;
        } else {
            printf(" \"%s\"", escaped);
            free(escaped);
        }
    }
    const xkb_keysym_t keysym = xkb_compose_table_entry_keysym(entry);
    if (keysym != XKB_KEY_NoSymbol) {
        xkb_keysym_get_name(keysym, buf, sizeof(buf));
        printf(" %s", buf);
    }
    printf("\n");
    return true;
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct xkb_context *ctx = NULL;
    struct xkb_compose_table *compose_table = NULL;
    const char *locale = NULL;
    const char *path = NULL;
    enum xkb_compose_format format = XKB_COMPOSE_FORMAT_TEXT_V1;
    enum options {
        OPT_FILE,
        OPT_LOCALE,
    };
    static struct option opts[] = {
        {"help",   no_argument,       0, 'h'},
        {"file",   required_argument, 0, OPT_FILE},
        {"locale", required_argument, 0, OPT_LOCALE},
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

    /* Initialize the locale to use */
    locale = setlocale(LC_CTYPE, NULL);
    if (!locale)
        locale = "C";

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_FILE:
            path = optarg;
            break;
        case OPT_LOCALE:
            locale = optarg;
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        case '?':
            usage(stderr, argv[0]);
            return EXIT_INVALID_USAGE;
        }
    }

    if (locale == NULL) {
        fprintf(stderr, "ERROR: Cannot determine the locale.\n");
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        fprintf(stderr, "Couldn't create xkb context\n");
        goto out;
    }

    if (path != NULL) {
        FILE *file = fopen(path, "rb");
        if (file == NULL) {
            perror(path);
            goto file_error;
        }
        compose_table =
            xkb_compose_table_new_from_file(ctx, file, locale, format,
                                            XKB_COMPOSE_COMPILE_NO_FLAGS);
        fclose(file);
        if (!compose_table) {
            fprintf(stderr, "Couldn't create compose from file: %s\n", path);
            goto out;
        }
    } else {
        compose_table =
            xkb_compose_table_new_from_locale(ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            fprintf(stderr,
                    "Couldn't create compose from locale \"%s\"\n", locale);
            goto out;
        }
    }

    struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(compose_table);
    struct xkb_compose_table_entry *entry;
    while ((entry = xkb_compose_table_iterator_next(iter))) {
        if (!print_compose_table_entry(entry)) {
            ret = EXIT_FAILURE;
            goto entry_error;
        }
    }
    ret = EXIT_SUCCESS;

entry_error:
    xkb_compose_table_iterator_free(iter);
out:
    xkb_compose_table_unref(compose_table);
file_error:
    xkb_context_unref(ctx);

    return ret;
}
