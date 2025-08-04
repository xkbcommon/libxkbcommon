/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <stdbool.h>

/* Don't use compat names in internal code. */
#define _XKBCOMMON_COMPAT_H
#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof(*(arr))))

/* Fields that are printed in the interactive tools. */
enum print_state_fields {
    PRINT_LAYOUT = (1u << 1),
    PRINT_UNICODE = (1u << 2),
    PRINT_ALL_FIELDS = ((PRINT_UNICODE << 1) - 1),
    /*
     * Fields that can be hidden with the option --short.
     * NOTE: If this value is modified, remember to update the documentation of
     *       the --short option in the corresponding tools.
     */
    PRINT_VERBOSE_FIELDS = (PRINT_LAYOUT | PRINT_UNICODE)
};
typedef uint32_t print_state_fields_mask_t;

void
print_modifiers_encodings(struct xkb_keymap *keymap);
void
print_keys_modmaps(struct xkb_keymap *keymap);

void
tools_print_keycode_state(const char *prefix,
                          struct xkb_state *state,
                          struct xkb_compose_state *compose_state,
                          xkb_keycode_t keycode,
                          enum xkb_key_direction direction,
                          enum xkb_consumed_mode consumed_mode,
                          print_state_fields_mask_t fields);

void
tools_print_state_changes(enum xkb_state_component changed);

void
tools_disable_stdin_echo(void);

void
tools_enable_stdin_echo(void);

const char *
select_backend(const char *wayland, const char *x11, const char *fallback);

int
tools_exec_command(const char *prefix, int argc, const char **argv);

bool
is_pipe_or_regular_file(int fd);

FILE*
tools_read_stdin(void);

#ifdef _WIN32
#define setenv(varname, value, overwrite) _putenv_s((varname), (value))
#define unsetenv(varname) _putenv_s(varname, "")
#endif
