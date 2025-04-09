/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#include "config.h"

#include "test.h"

#ifdef _WIN32

main()
{
    test_init();
    return SKIP_TEST;
}

#else

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"

#include "test.h"
#include "utils.h"
#include "src/keysym.h"

static void
test_unicode_keysyms_consistency(uint32_t start, uint32_t end)
{
    static_assert(XKB_KEYSYM_NAME_MAX_SIZE > XKB_KEYSYM_UTF8_MAX_SIZE,
                  "Buffer too small");
    char buffer[XKB_KEYSYM_NAME_MAX_SIZE] = {0};
    char utf8[XKB_KEYSYM_UTF8_MAX_SIZE] = {0};
    for (uint32_t cp = start; cp <= end; cp++) {
        xkb_keysym_t unicode = XKB_KEYSYM_UNICODE_OFFSET + cp;
        xkb_keysym_t canonical = xkb_utf32_to_keysym(cp);
        int count = xkb_keysym_get_name(unicode, buffer, sizeof(buffer));
        assert(count > 0);
        if (cp == 0 || is_surrogate(cp)) {
            /*
             * Invalid code points
             */

            /* No conversion from code point */
            assert(canonical == XKB_KEY_NoSymbol);
            /* No conversion to code point */
            assert(xkb_keysym_to_utf32(unicode) == 0);
            char *end_ptr = NULL;
            if (cp == 0) {
                /* Corresponding name hexadecimal format, unchanged */
                assert(strtoull(buffer, &end_ptr, 16) == unicode);
            } else {
                /* Unicode notation */
                assert(count == 5 && buffer[0] == 'U');
                assert(strtoull(buffer + 1, &end_ptr, 16) == cp);
            }
            assert(*end_ptr == '\0');
            /* Roundtrip hexadecimal name */
            xkb_keysym_t ks = xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS);
            assert(ks == unicode);
            /* Check Unicode format */
            snprintf(buffer, sizeof(buffer), "U%"PRIX32, cp);
            ks = xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS);
            assert((cp == 0 && ks == XKB_KEY_NoSymbol) ^
                   (is_surrogate(cp) && ks == unicode));
            /* Cannot convert to UTF-8 */
            count = xkb_keysym_to_utf8(unicode, buffer, sizeof(buffer));
            assert(count == 0);
        } else {
            /*
             * Valid code points
             */

            /* Canonical keysym may be different but is set */
            assert_printf((canonical == unicode) ^
                          ((0x20 <= cp && cp <= 0x100 && cp != 0x7f && canonical == cp) ||
                           (canonical != unicode &&
                            canonical != XKB_KEY_NoSymbol &&
                            (canonical != cp || canonical == XKB_KEY_EuroSign))),
                          "Code point: U+%04"PRIX32", Unicode: %#"PRIx32", "
                          "canonical: %#"PRIx32"\n",
                          cp, unicode, canonical);
            /* Conversion to code point has the same expected result */
            assert(xkb_keysym_to_utf32(unicode) == cp);
            assert(xkb_keysym_to_utf32(canonical) == cp); /* roundtrip */
            /* Check name roundtrip */
            xkb_keysym_t ks = xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS);
            assert((unicode != canonical && ks == canonical) ^ (ks == unicode));
            /* Can use Unicode format */
            if (buffer[0] == 'U' && count > 4 && is_digit(buffer[1])) {
                /* Name already a Unicode notation: skip.
                 * Note that the heuristic may fail to detect a Unicode notation,
                 * but it is only meant to make the overall test faster. */
            } else {
                snprintf(buffer, sizeof(buffer), "U%"PRIX32, cp);
                ks = xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS);
                assert((unicode != canonical && ks == canonical) ^ (ks == unicode));
            }
            /* Roundtrip: numeric hexadecimal format for Unicode keysym */
            assert(snprintf(buffer, sizeof(buffer), "%#"PRIx32, unicode) == 9);
            ks = xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS);
            assert(ks == unicode);
            /* Can convert to UTF-8 (Unicode keysym) */
            count = xkb_keysym_to_utf8(unicode, buffer, sizeof(buffer));
            assert(count > 0);
            if (canonical != unicode) {
                /* Canonical keysym convert to same UTF-8 */
                const int count2 = xkb_keysym_to_utf8(canonical, utf8, sizeof(utf8));
                assert(count2 == count);
                assert(strcmp(buffer, utf8) == 0);
                /* Roundtrip: numeric hexadecimal format for canonical keysym */
                count = xkb_keysym_get_name(canonical, buffer, sizeof(buffer));
                assert(count > 0);
                assert(xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS) == canonical);
                assert(snprintf(buffer, sizeof(buffer), "%#"PRIx32, canonical) > 2);
                assert(xkb_keysym_from_name(buffer, XKB_KEYSYM_NO_FLAGS) == canonical);
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    test_init();

    unsigned long int NUM_PROCESSES = 0;
    if (argc > 1) {
        NUM_PROCESSES = strtoul(argv[1], NULL, 10);
    }
    if (NUM_PROCESSES == 0 || NUM_PROCESSES > 32)
        NUM_PROCESSES = 4;

    const uint32_t max_codepoint = 0x10ffff;
    const uint32_t chunk = max_codepoint / NUM_PROCESSES;
    pid_t pids[NUM_PROCESSES];

    for (unsigned long int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            /* Child */
            const uint32_t start = i * chunk;
            const uint32_t end = (i == NUM_PROCESSES - 1)
                               ? max_codepoint
                               : start + chunk;
            test_unicode_keysyms_consistency(start, end);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            /* Parent */
            pids[i] = pid;
        } else {
            perror("fork");
            return TEST_SETUP_FAILURE;
        }
    }

    /* Wait for children */
    int exit_code = EXIT_SUCCESS;
    for (unsigned long int i = 0; i < NUM_PROCESSES; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            exit_code = EXIT_FAILURE;
        } else if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code != EXIT_SUCCESS) {
                fprintf(stderr,
                        "Child PID %d exited with code %d\n",
                        pids[i], code);
                exit_code = EXIT_FAILURE;
            }
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr,
                    "Child PID %d terminated by signal %d\n",
                    pids[i], WTERMSIG(status));
            exit_code = EXIT_FAILURE;
        }
    }

    return exit_code;
}
#endif
