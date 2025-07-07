/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

/*
 *This is a wrapper around X11 tests to make it faster to use for the simple
 * type of test cases.
 *
 * Use with the X11_TEST macro like this:
 *
 * X11_TEST(some_test) {
 *  return 0;
 * }
 *
 * int main(void) {
 *  return x11_tests_run(void);
 * }
 *
 */

#pragma once

#include "config.h"

int xvfb_wrapper(int (*f)(char* display));

int x11_tests_run(void);

typedef int (* x11_test_func_t)(char* display);

struct test_function {
    const char *name;     /* function name */
    const char *file;     /* file name */
    x11_test_func_t func; /* test function */
} __attribute__((aligned(16)));

/**
 * Defines a struct test_function in a custom ELF section that we can then
 * loop over in x11_tests_run() to extract the tests. This removes the
 * need of manually adding the tests to a suite or listing them somewhere.
 */
#define TEST_ELF_SECTION test_func_sec

#if defined(__APPLE__) && defined(__MACH__)
#define SET_TEST_ELF_SECTION(_section) \
    __attribute__((retain,used)) \
    __attribute__((section("__DATA," STRINGIFY(_section))))

/* Custom section pointers. See: https://stackoverflow.com/a/22366882 */
#define DECLARE_TEST_ELF_SECTION_POINTERS(_section) \
extern const struct test_function CONCAT2(__start_, _section) \
    __asm("section$start$__DATA$" STRINGIFY2(_section)); \
extern const struct test_function CONCAT2(__stop_, _section) \
    __asm("section$end$__DATA$" STRINGIFY2(_section))

#else

#define SET_TEST_ELF_SECTION(_section) \
    __attribute__((retain,used)) \
    __attribute__((section(STRINGIFY(_section))))

#define DECLARE_TEST_ELF_SECTION_POINTERS(_section) \
    extern const struct test_function \
    CONCAT2(__start_, _section), CONCAT2(__stop_, _section)
#endif

#define X11_TEST(_func) \
static int _func(char* display); \
static const struct test_function _test_##_func \
SET_TEST_ELF_SECTION(TEST_ELF_SECTION) = { \
    .name = #_func, \
    .func = (_func), \
    .file = __FILE__, \
}; \
static int _func(char* display)
