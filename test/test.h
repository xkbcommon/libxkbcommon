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
#include "utils.h"

/* Automake test exit code to signify SKIP (à la PASS, FAIL, etc).
 * See: https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html */
#define SKIP_TEST 77
#define TEST_SETUP_FAILURE 99

#define assert_printf(cond, ...) do {                      \
    const bool __cond = (cond);                            \
    if (!__cond) {                                         \
       fprintf(stderr, "Assertion failure: " __VA_ARGS__); \
       assert(__cond);                                     \
    }} while (0)

#define assert_streq_not_null(test_name, expected, got) \
    assert_printf(streq_not_null(expected, got), \
                  test_name ". Expected \"%s\", got: \"%s\"\n", expected, got)

#define assert_eq(test_name, expected, got, format, ...) \
    assert_printf(expected == got, \
                  test_name ". Expected " format ", got: " format "\n", \
                  __VA_ARGS__, expected, got)

void
test_init(void);

void
print_detailed_state(struct xkb_state *state);

/* The offset between KEY_* numbering, and keycodes in the XKB evdev
 * dataset. */
#define EVDEV_OFFSET 8

enum key_seq_state {
    DOWN,
    REPEAT,
    UP,
    BOTH,
    NEXT,
    FINISH,
};

int
test_key_seq(struct xkb_keymap *keymap, ...);

int
test_key_seq_va(struct xkb_keymap *keymap, va_list args);

char *
test_makedir(const char *parent, const char *path);

char *
test_maketempdir(const char *template);

char *
test_get_path(const char *path_rel);

char *
test_read_file(const char *path_rel);

enum test_context_flags {
    CONTEXT_NO_FLAG = 0,
    CONTEXT_ALLOW_ENVIRONMENT_NAMES = (1 << 0),
};

struct xkb_context *
test_get_context(enum test_context_flags flags);

struct xkb_keymap *
test_compile_file(struct xkb_context *context, enum xkb_keymap_format format,
                  const char *path_rel);

struct xkb_keymap *
test_compile_string(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *string);

struct xkb_keymap *
test_compile_buffer(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *buf, size_t len);

typedef struct xkb_keymap * (*test_compile_buffer_t)(struct xkb_context *context,
                                                     enum xkb_keymap_format format,
                                                     const char *buf, size_t len,
                                                     void *private);

bool
test_compile_output(struct xkb_context *ctx, enum xkb_keymap_format input_format,
                    enum xkb_keymap_format output_format,
                    test_compile_buffer_t compile_buffer,
                    void *compile_buffer_private, const char *test_title,
                    const char *keymap_str, size_t keymap_len,
                    const char *rel_path, bool update_output_files);

typedef int (*test_third_party_compile_buffer_t)(const char *buf, size_t len,
                                                 void *private,
                                                 char **out, size_t *size_out);

bool
test_third_pary_compile_output(test_third_party_compile_buffer_t compile_buffer,
                               void *compile_buffer_private,
                               const char *test_title,
                               const char *keymap_str, size_t keymap_len,
                               const char *rel_path, bool update_output_files);

struct xkb_keymap *
test_compile_rmlvo(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options);

struct xkb_keymap *
test_compile_rules(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options);


#ifdef _WIN32
#define setenv(varname, value, overwrite) _putenv_s((varname), (value))
#define unsetenv(varname) _putenv_s(varname, "")
#endif
