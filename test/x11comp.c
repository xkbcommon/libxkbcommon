/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
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

#include <stdio.h>
#include <spawn.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "test.h"
#include "test/utils-text.h"
#define XKBCOMP_COMPATIBILITY
#include "test/merge_modes.h"
#include "xvfb-wrapper.h"
#include "xkbcommon/xkbcommon-x11.h"

static enum update_files update_test_files = NO_UPDATE;

/* Reset keymap to `empty` on the server.
 * It seems that xkbcomp does not fully set the keymap on the server and
 * the conflicting leftovers may raises errors. */
static int
reset_keymap(const char* display)
{
    char *envp[] = { NULL };
    const char* setxkbmap_argv[] = {
        "setxkbmap", "-display", display,
        "-geometry", "pc",
        "-keycodes", "evdev",
        "-compat", "basic",
        "-types", "basic+numpad", // Avoid extra types
        "-symbols", "us",
        // "-v", "10",
        NULL
    };

    /* Launch xkbcomp */
    pid_t setxkbmap_pid;
    int ret = posix_spawnp(&setxkbmap_pid, "setxkbmap", NULL, NULL,
                           (char* const*)setxkbmap_argv, envp);
    if (ret != 0) {
        perror("posix_spawnp xkbcomp failed.");
        return TEST_SETUP_FAILURE;
    }
    int status;
    ret = waitpid(setxkbmap_pid, &status, 0);
    return (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        ? TEST_SETUP_FAILURE
        : EXIT_SUCCESS;
}

static int
run_xkbcomp_file(const char* display, xcb_connection_t *conn, int32_t device_id,
                 const char *keymap_path)
{
    /* Prepare xkbcomp parameters */
    char *envp[] = { NULL };
    char *xkb_path = test_get_path(keymap_path);
    assert(xkb_path);
    const char* xkbcomp_argv[] = {"xkbcomp", "-I", xkb_path, display, NULL};

    /* Launch xkbcomp */
    pid_t xkbcomp_pid;
    int ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", NULL, NULL,
                           (char* const*)xkbcomp_argv, envp);
    free(xkb_path);
    if (ret != 0) {
        perror("posix_spawnp xkbcomp failed.");
        return TEST_SETUP_FAILURE;
    }

    /* Wait for xkbcomp to complete */
    int status;
    ret = waitpid(xkbcomp_pid, &status, 0);
    return (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        ? TEST_SETUP_FAILURE
        : EXIT_SUCCESS;
}

#define PIPE_WRITE 1
#define PIPE_READ 0

static int
run_xkbcomp_str(const char* display, xcb_connection_t *conn, int32_t device_id,
                const char *include_path, const char *keymap)
{
    int ret = EXIT_FAILURE;
    assert(keymap);

    /* Prepare xkbcomp parameters */
    char *envp[] = { NULL };
    char include_path_arg[PATH_MAX+2] = "-I";
    const char *xkbcomp_argv[] = {
        "xkbcomp", "-I" /* reset include path*/, include_path_arg,
        "-opt", "g", "-w", "10", "-" /* stdin */, display, NULL
    };
    if (include_path) {
        ret = snprintf(include_path_arg, ARRAY_SIZE(include_path_arg),
                       "-I%s", include_path);
        if (ret < 0 || ret >= (int)ARRAY_SIZE(include_path_arg)) {
            return TEST_SETUP_FAILURE;
        }
    }

    /* Prepare input */
    int stdin_pipe[2];
    posix_spawn_file_actions_t action;
    if (pipe(stdin_pipe) == -1) {
        perror("pipe");
        ret = TEST_SETUP_FAILURE;
        goto pipe_error;
    }
    if (posix_spawn_file_actions_init(&action)) {
        perror("spawn_file_actions_init error");
        goto posix_spawn_file_actions_init_error;
    }

    /* Make spawned process close unused write-end of pipe, else it will not
     * receive EOF when write-end of the pipe is closed below and it will result
     * in a deadlock. */
    if (posix_spawn_file_actions_addclose(&action, stdin_pipe[PIPE_WRITE]))
        goto posix_spawn_file_actions_error;
    /* Make spawned process replace stdin with read end of the pipe */
    if (posix_spawn_file_actions_adddup2(&action, stdin_pipe[PIPE_READ], STDIN_FILENO))
        goto posix_spawn_file_actions_error;

    /* Launch xkbcomp */
    pid_t xkbcomp_pid;
    ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", &action, NULL,
                       (char* const*)xkbcomp_argv, envp);
    if (ret != 0) {
        errno = ret;
        perror("posix_spawnp xkbcomp failed");
        goto posix_spawn_file_actions_init_error;
    }
    /* Close unused read-end of pipe */
    close(stdin_pipe[PIPE_READ]);
    ret = write(stdin_pipe[PIPE_WRITE], keymap, strlen(keymap));
    /* Close write-end of the pipe, to emit EOF */
    close(stdin_pipe[PIPE_WRITE]);
    if (ret == -1) {
        perror("Cannot write keymap to stdin");
        kill(xkbcomp_pid, SIGTERM);
    }

    /* Wait for xkbcomp to complete */
    int status;
    ret = waitpid(xkbcomp_pid, &status, 0);
    ret = (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        ? TEST_SETUP_FAILURE
        : EXIT_SUCCESS;
    goto cleanup;

posix_spawn_file_actions_error:
    perror("posix_spawn_file_actions_* error");
posix_spawn_file_actions_init_error:
    close(stdin_pipe[PIPE_WRITE]);
    close(stdin_pipe[PIPE_READ]);
    ret = TEST_SETUP_FAILURE;
cleanup:
    posix_spawn_file_actions_destroy(&action);
pipe_error:
    return ret;
}

static char *
prepare_keymap_file(const char *keymap_path, bool strip_all_comments)
{
    char *original = test_read_file(keymap_path);
    assert(original);
    char *tmp;
    if (strip_all_comments) {
        tmp = original;
    } else {
        tmp = uncomment(original, strlen(original), "//");
        free(original);
        assert(tmp);
    }
    char *expected = strip_lines(tmp, strlen(tmp), "//");
    free(tmp);
    return expected;
}

struct compile_buffer_private {
    const char* display;
    xcb_connection_t *conn;
    int32_t device_id;
};

static struct xkb_keymap *
compile_buffer(struct xkb_context *ctx, const char *buf, size_t len,
               void *private)
{
    const struct compile_buffer_private *config = private;
    assert(buf[len - 1] == '\0');

    char *include_path = test_get_path("");
    int ret = run_xkbcomp_str(config->display, config->conn,
                              config->device_id, include_path, buf);
    free(include_path);

    if (ret != EXIT_SUCCESS)
        return NULL;

    return xkb_x11_keymap_new_from_device(ctx, config->conn, config->device_id,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
}

static int
test_keymap_roundtrip(const char* display, xcb_connection_t *conn,
                      int32_t device_id, const char *keymap_path)
{
    /* Get original keymap */
    char* expected = prepare_keymap_file(keymap_path, false);

    int ret = reset_keymap(display);
    if (ret != EXIT_SUCCESS)
        return ret;

    /* Load keymap into X11 */
    ret = run_xkbcomp_file(display, conn, device_id, keymap_path);
    if (ret != EXIT_SUCCESS)
        goto xkbcomp_error;

    /* Get keymap from X11 */
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);
    struct xkb_keymap *keymap =
        xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);

    /* Dump keymap and compare to expected */
    char *got = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(got);

    if (!streq(expected, got)) {
        fprintf(stderr,
                "round-trip test failed: dumped map differs from expected\n");
        fprintf(stderr, "length: dumped %lu, expected %lu\n",
                (unsigned long) strlen(got),
                (unsigned long) strlen(expected));
        fprintf(stderr, "dumped map:\n");
        fprintf(stderr, "%s\n", got);
        ret = EXIT_FAILURE;
    } else {
        ret = EXIT_SUCCESS;
    }

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    free(got);
xkbcomp_error:
    free(expected);
    return ret;
}

static int
init_x11_test(const char *display, xcb_connection_t **conn_rtrn,
              int32_t *device_id_rtrn)
{
    xcb_connection_t *conn = xcb_connect(display, NULL);
    if (xcb_connection_has_error(conn)) {
        return TEST_SETUP_FAILURE;
    }
    int ret = xkb_x11_setup_xkb_extension(conn,
                                          XKB_X11_MIN_MAJOR_XKB_VERSION,
                                          XKB_X11_MIN_MINOR_XKB_VERSION,
                                          XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                          NULL, NULL, NULL, NULL);
    if (!ret)
        goto err_xcb;
    int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1)
        goto err_xcb;
    *conn_rtrn = conn;
    *device_id_rtrn = device_id;
    return EXIT_SUCCESS;

err_xcb:
    xcb_disconnect(conn);
    return TEST_SETUP_FAILURE;
}

X11_TEST(test_basic)
{
    if (update_test_files != NO_UPDATE)
        return EXIT_SUCCESS;

    xcb_connection_t *conn = NULL;
    int32_t device_id = 0;
    int ret = init_x11_test(display, &conn, &device_id);
    if (ret != EXIT_SUCCESS) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }

    /* Test some keymaps */
    const char *keymaps[] = {
        "keymaps/host.xkb",
    };
    for (unsigned k = 0; k < ARRAY_SIZE(keymaps); k++) {
        ret = test_keymap_roundtrip(display, conn, device_id, keymaps[k]);
        if (ret != EXIT_SUCCESS)
            break;
    }

err_xcb:
    xcb_disconnect(conn);
    return ret;
}

X11_TEST(test_merge_modes)
{
    xcb_connection_t *conn = NULL;
    int32_t device_id = 0;
    int ret = init_x11_test(display, &conn, &device_id);
    if (ret != EXIT_SUCCESS) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }

    ret = reset_keymap(display);
    if (ret != EXIT_SUCCESS)
        goto err_xcb;

    /* Test merge modes */
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);
    const struct compile_buffer_private compile_buffer_config = {
        .display = display,
        .conn = conn,
        .device_id = device_id
    };

    make_symbols_tests("merge_modes_x11", "", "x11",
                       compile_buffer, (void*)&compile_buffer_config, update_test_files);

    xkb_context_unref(ctx);
    ret = EXIT_SUCCESS;

err_xcb:
    xcb_disconnect(conn);
    return ret;
}

struct xkbcomp_roundtrip_data {
    const char *path;
    bool tweak_comments;
};

static int
xkbcomp_roundtrip(const char *display, void *private) {
    const struct xkbcomp_roundtrip_data *priv = private;
    int ret;

    xcb_connection_t *conn = xcb_connect(display, NULL);
    if (xcb_connection_has_error(conn)) {
        ret = TEST_SETUP_FAILURE;
        goto err_conn;
    }
    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }
    int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }

    char* original = prepare_keymap_file(priv->path, priv->tweak_comments);
    if (!original) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }
    ret = reset_keymap(display);
    if (ret != EXIT_SUCCESS) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }
    ret = run_xkbcomp_str(display, conn, device_id, NULL, original);
    if (ret != EXIT_SUCCESS) {
        fprintf(stderr, "ERROR: xkbcomp failed.\n");
        goto xkbcomp_error;
    }

    /* Get keymap from X11 */
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        ret = EXIT_FAILURE;
        goto context_error;
    }
    struct xkb_keymap *keymap =
        xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        ret = EXIT_FAILURE;
        fprintf(stderr, "ERROR: Failed to get keymap from X server.\n");
        goto keymap_error;
    }

    /* Dump keymap and compare to expected */
    char *got = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    if (!got) {
        ret = EXIT_FAILURE;
        fprintf(stderr, "ERROR: Failed to dump keymap.\n");
    } else {
        fprintf(stdout, "%s\n", got);
        if (strcmp(got, original) != 0) {
            ret = EXIT_FAILURE;
            fprintf(stderr, "ERROR: roundtrip failed. Got %zu, expected %zu.\n",
                    strlen(got), strlen(original));
        } else {
            ret = EXIT_SUCCESS;
            fprintf(stderr, "Roundtrip succeed.\n");
        }
        free(got);
    }

    xkb_keymap_unref(keymap);
keymap_error:
    xkb_context_unref(ctx);
context_error:
xkbcomp_error:
    free(original);
err_xcb:
    xcb_disconnect(conn);
err_conn:
    return ret;
}

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [--update] [--update-init] "
            "[--keymap <path to keymap file>] [--tweak] [--help]\n",
            progname);
}

int
main(int argc, char **argv) {
    test_init();

    enum options {
        OPT_UPDATE_GOLDEN_TEST,
        OPT_UPDATE_GOLDEN_TEST_INIT,
        OPT_FILE,
        OPT_TWEAK_COMMENTS,
    };
    static struct option opts[] = {
        {"help",        no_argument,       0, 'h'},
        {"update",      no_argument,       0, OPT_UPDATE_GOLDEN_TEST},
        {"update-init", no_argument,       0, OPT_UPDATE_GOLDEN_TEST_INIT},
        {"keymap",      required_argument, 0, OPT_FILE},
        {"tweak",       no_argument,       0, OPT_TWEAK_COMMENTS},
        {0, 0, 0, 0},
    };

    bool tweak_comments = false;
    const char *path = NULL;

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_UPDATE_GOLDEN_TEST:
            update_test_files = UPDATE_USING_TEST_OUTPUT;
            break;
        case OPT_UPDATE_GOLDEN_TEST_INIT:
            update_test_files = UPDATE_USING_TEST_INPUT;
            break;
        case OPT_FILE:
            path = optarg;
            break;
        case OPT_TWEAK_COMMENTS:
            tweak_comments = optarg;
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        case '?':
            usage(stderr, argv[0]);
            return EXIT_INVALID_USAGE;
        }
    }

    if (update_test_files != NO_UPDATE && path != NULL) {
        fprintf(stderr,
                "ERROR: --update* and --keymap are mutually exclusive.\n");
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    if (path == NULL) {
        return x11_tests_run();
    } else {
        struct xkbcomp_roundtrip_data priv = {
            .path = path,
            .tweak_comments = tweak_comments
        };
        return xvfb_wrapper(xkbcomp_roundtrip, &priv);
    }
}
