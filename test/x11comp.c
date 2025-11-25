/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-names.h"
#include "xkbcommon/xkbcommon-x11.h"

#include "evdev-scancodes.h"
#include "test.h"
#include "tools/tools-common.h"
#include "utils.h"
#include "utils-text.h"
#include "xvfb-wrapper.h"

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

enum update_files {
    NO_UPDATE = 0,
    UPDATE_USING_TEST_INPUT,
    UPDATE_USING_TEST_OUTPUT
};

static enum update_files update_test_files = NO_UPDATE;


static char *
prepare_keymap(const char *original, bool strip_all_comments)
{
    if (strip_all_comments) {
        return strip_lines(original, strlen(original), "//");
    } else {
        char *tmp = uncomment(original, strlen(original), "//");
        char *ret = strip_lines(tmp, strlen(tmp), "//");
        free(tmp);
        return ret;
    }
}

/*
 * Reset keymap to `empty` on the server.
 * It seems that xkbcomp does not fully set the keymap on the server and
 * the conflicting leftovers may raise errors.
 */
static int
reset_keymap(const char* display)
{
    char *envp[] = { NULL };
    const char* setxkbmap_argv[] = {
        "setxkbmap", "-display", display,
        "-geometry", "pc",
        "-keycodes", "evdev",
        "-compat", "basic",
        "-types", "basic+numpad", /* Avoid extra types */
        "-symbols", "us",
        // "-v", "10",
        NULL
    };

    /* Launch xkbcomp */
    pid_t setxkbmap_pid;
    int ret = posix_spawnp(&setxkbmap_pid, "setxkbmap", NULL, NULL,
                           (char* const*)setxkbmap_argv, envp);
    if (ret != 0) {
        fprintf(stderr,
                "[ERROR] Cannot run setxkbmap. posix_spawnp error %d: %s\n",
                ret, strerror(ret));
        if (ret == ENOENT) {
            fprintf(stderr,
                    "[ERROR] setxkbmap may be missing. "
                    "Please install the corresponding package, "
                    "e.g. \"setxkbmap\" or \"x11-xkb-utils\".\n");
        }
        return TEST_SETUP_FAILURE;
    }
    int status;
    ret = waitpid(setxkbmap_pid, &status, 0);
    return (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        ? TEST_SETUP_FAILURE
        : EXIT_SUCCESS;
}

// TODO: unused for now
// /* Use xkbcomp to load a keymap from a file */
// static int
// run_xkbcomp_file(const char* display, xcb_connection_t *conn, int32_t device_id,
//                  const char *xkb_path)
// {
//     /* Prepare xkbcomp parameters */
//     char *envp[] = { NULL };
//     const char* xkbcomp_argv[] = {"xkbcomp", "-I", xkb_path, display, NULL};
//
//     /* Launch xkbcomp */
//     pid_t xkbcomp_pid;
//     int ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", NULL, NULL,
//                            (char* const*)xkbcomp_argv, envp);
//     if (ret != 0) {
//         fprintf(stderr,
//                 "[ERROR] Cannot run xkbcomp. posix_spawnp error %d: %s\n",
//                 ret, strerror(ret));
//         if (ret == ENOENT) {
//             fprintf(stderr,
//                     "[ERROR] xkbcomp may be missing. "
//                     "Please install the corresponding package, "
//                     "e.g. \"xkbcomp\" or \"x11-xkb-utils\".\n");
//         }
//         return TEST_SETUP_FAILURE;
//     }
//
//     /* Wait for xkbcomp to complete */
//     int status;
//     ret = waitpid(xkbcomp_pid, &status, 0);
//     return (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
//         ? TEST_SETUP_FAILURE
//         : EXIT_SUCCESS;
// }

enum {
    PIPE_READ  = 0,
    PIPE_WRITE = 1
};

/* Use xkbcomp to load a keymap from a string */
static int
run_xkbcomp_str(const char* display, const char *include_path, const char *keymap)
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
        fprintf(stderr,
                "[ERROR] Cannot run xkbcomp. posix_spawnp error %d: %s\n",
                ret, strerror(ret));
        if (ret == ENOENT) {
            fprintf(stderr,
                    "[ERROR] xkbcomp may be missing. "
                    "Please install the corresponding package, "
                    "e.g. \"xkbcomp\" or \"x11-xkb-utils\".\n");
        }
        goto posix_spawn_file_actions_init_error;
    }
    /* Close unused read-end of pipe */
    close(stdin_pipe[PIPE_READ]);
    const ssize_t count = write(stdin_pipe[PIPE_WRITE], keymap, strlen(keymap));
    /* Close write-end of the pipe, to emit EOF */
    close(stdin_pipe[PIPE_WRITE]);
    if (count == -1) {
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

// TODO: implement tests to ensure compatibility with xkbcomp
// struct compile_buffer_private {
//     const char* display;
//     xcb_connection_t *conn;
//     int32_t device_id;
// };
//
// static struct xkb_keymap *
// compile_buffer(struct xkb_context *ctx, const char *buf, size_t len,
//                void *private)
// {
//     const struct compile_buffer_private *config = private;
//     assert(buf[len - 1] == '\0');
//
//     char *include_path = test_get_path("");
//     int ret = run_xkbcomp_str(config->display, config->conn,
//                               config->device_id, include_path, buf);
//     free(include_path);
//
//     if (ret != EXIT_SUCCESS)
//         return NULL;
//
//     return xkb_x11_keymap_new_from_device(ctx, config->conn, config->device_id,
//                                           XKB_KEYMAP_COMPILE_NO_FLAGS);
// }

static int
test_keymap_roundtrip(struct xkb_context *ctx,
                      const char* display, xcb_connection_t *conn,
                      int32_t device_id, bool print_keymap, bool tweak,
                      enum xkb_keymap_serialize_flags serialize_flags,
                      const char *keymap_path)
{
    /* Get raw keymap */
    FILE *file = NULL;
    if (isempty(keymap_path) || strcmp(keymap_path, "-") == 0) {
        /* Read stdin */
        file = tools_read_stdin();
        if (!file)
            return TEST_SETUP_FAILURE;
    } else {
        /* Read file from path */
        file = fopen(keymap_path, "rb");
        if (!file) {
            perror("Unable to read file");
            return TEST_SETUP_FAILURE;
        }
    }
    char *original = read_file(keymap_path, file);
    fclose(file);
    if (!original)
        return TEST_SETUP_FAILURE;

    /* Pre-process keymap string */
    char* expected = prepare_keymap(original, tweak);
    free(original);
    if (!expected) {
        return TEST_SETUP_FAILURE;
    }

    /* Prepare X server */
    int ret = reset_keymap(display);
    if (ret != EXIT_SUCCESS) {
#ifdef __APPLE__
        /* Brew may not provide setxkbmap */
#else
        goto xkbcomp_error;
#endif
    }

    /* Load keymap into X server */
    ret = run_xkbcomp_str(display, NULL, expected);
    if (ret != EXIT_SUCCESS)
        goto xkbcomp_error;

    /* Get keymap from X server */
    struct xkb_keymap *keymap =
        xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);
    if (!keymap) {
        ret = EXIT_FAILURE;
        fprintf(stderr, "ERROR: Failed to get keymap from X server.\n");
        goto xkbcomp_error;
    }

    /* Dump keymap and compare to expected */
    char *got = xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                          serialize_flags);
    if (!got) {
        ret = EXIT_FAILURE;
        fprintf(stderr, "ERROR: Failed to dump keymap.\n");
    } else {
        if (print_keymap)
            fprintf(stdout, "%s\n", got);
        if (!streq(got, expected)) {
            ret = EXIT_FAILURE;
            fprintf(stderr,
                    "ERROR: roundtrip failed. "
                    "Lengths difference: got %zu, expected %zu.\n",
                    strlen(got), strlen(expected));
        } else {
            ret = EXIT_SUCCESS;
            fprintf(stderr, "Roundtrip succeed.\n");
        }
        free(got);
    }

    xkb_keymap_unref(keymap);
xkbcomp_error:
    free(expected);
    return ret;
}

static int
init_x11_connection(const char *display, xcb_connection_t **conn_rtrn,
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
    int ret = init_x11_connection(display, &conn, &device_id);
    if (ret != EXIT_SUCCESS) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    static const struct {
        const char *path;
        enum xkb_keymap_serialize_flags serialize_flags;
    } keymaps[] = {
        {
            .path = "keymaps/host-no-pretty.xkb",
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
                             & ~XKB_KEYMAP_SERIALIZE_PRETTY
        },
        /* This last keymap will be used for the next tests */
        {
            .path = "keymaps/host.xkb",
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
        },
    };
    for (size_t k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        char *keymap_path = test_get_path(keymaps[k].path);
        assert(keymap_path);

        ret = test_keymap_roundtrip(ctx, display, conn, device_id,
                                    false, false, keymaps[k].serialize_flags,
                                    keymap_path);
        assert(ret == EXIT_SUCCESS);

        free(keymap_path);
    }

    struct xkb_keymap *keymap =
        xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);

    /* Check capitalization transformation */
    struct xkb_state *state =
        xkb_x11_state_new_from_device(keymap, conn, device_id);
    assert(state);
    xkb_keysym_t sym;
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_a);
    sym = xkb_state_key_get_one_sym(state, KEY_LEFT + EVDEV_OFFSET);
    assert(sym == XKB_KEY_Left);
    const xkb_mod_index_t caps_idx =
        xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    assert(caps_idx != XKB_MOD_INVALID);
    const xkb_mod_mask_t caps = UINT32_C(1) << caps_idx;
    xkb_state_update_mask(state, 0, 0, caps, 0, 0, 0);
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_A);
    sym = xkb_state_key_get_one_sym(state, KEY_LEFT + EVDEV_OFFSET);
    assert(sym == XKB_KEY_Left);
    xkb_state_unref(state);

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    ret = EXIT_SUCCESS;

err_xcb:
    if (conn != NULL)
        xcb_disconnect(conn);
    return ret;
}

struct xkbcomp_roundtrip_data {
    const char *path;
    bool tweak_comments;
    enum xkb_keymap_serialize_flags serialize_flags;
};

static int
xkbcomp_roundtrip(const char *display, void *private) {
    const struct xkbcomp_roundtrip_data *priv = private;

    xcb_connection_t *conn = NULL;
    int32_t device_id = 0;
    int ret = init_x11_connection(display, &conn, &device_id);
    if (ret != EXIT_SUCCESS) {
        ret = TEST_SETUP_FAILURE;
        goto error_xcb;
    }

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        ret = EXIT_FAILURE;
        goto error_context;
    }

    ret = test_keymap_roundtrip(ctx, display, conn, device_id, true,
                                priv->tweak_comments, priv->serialize_flags,
                                priv->path);

    xkb_context_unref(ctx);
error_context:
error_xcb:
    xcb_disconnect(conn);
    return ret;
}

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp,
            "Usage: %s [--update] [--update-obtained] "
            "[--keymap KEYMAP_FILE] [--tweak] [--no-pretty] [--help]\n",
            progname);
}

int
main(int argc, char **argv) {
    test_init();

    enum options {
        OPT_UPDATE_GOLDEN_TEST_WITH_OUTPUT,
        OPT_UPDATE_GOLDEN_TEST_WITH_INPUT,
        OPT_FILE,
        OPT_TWEAK_COMMENTS,
        OPT_NO_PRETTY,
    };
    static struct option opts[] = {
        {"help",            no_argument,       0, 'h'},
        {"update-obtained", no_argument,       0, OPT_UPDATE_GOLDEN_TEST_WITH_OUTPUT},
        {"update",          no_argument,       0, OPT_UPDATE_GOLDEN_TEST_WITH_INPUT},
        {"keymap",          required_argument, 0, OPT_FILE},
        {"tweak",           no_argument,       0, OPT_TWEAK_COMMENTS},
        {"no-pretty",       no_argument,       0, OPT_NO_PRETTY},
        {0, 0, 0, 0},
    };

    bool tweak_comments = false;
    const char *path = NULL;
    enum xkb_keymap_serialize_flags serialize_flags =
        (enum xkb_keymap_serialize_flags) TEST_KEYMAP_SERIALIZE_FLAGS;

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_UPDATE_GOLDEN_TEST_WITH_OUTPUT:
            update_test_files = UPDATE_USING_TEST_OUTPUT;
            break;
        case OPT_UPDATE_GOLDEN_TEST_WITH_INPUT:
            update_test_files = UPDATE_USING_TEST_INPUT;
            break;
        case OPT_FILE:
            path = optarg;
            break;
        case OPT_TWEAK_COMMENTS:
            tweak_comments = optarg;
            break;
        case OPT_NO_PRETTY:
            serialize_flags &= ~XKB_KEYMAP_SERIALIZE_PRETTY;
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        default:
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
            .tweak_comments = tweak_comments,
            .serialize_flags = serialize_flags
        };
        return xvfb_wrapper(xkbcomp_roundtrip, &priv);
    }
}
