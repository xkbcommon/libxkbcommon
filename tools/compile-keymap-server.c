/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
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

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "xkbcommon/xkbcommon.h"
#include "src/context.h"
#ifdef ENABLE_KEYMAP_CACHE
#include "src/xkbcomp/cache.h"
#endif
#include "tools-common.h"
#include "src/utils.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"

static bool verbose = false;
static const char *includes[64];
static size_t num_includes = 0;

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Start a server to compile keymaps\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           " --socket <path>\n"
           "    Path of the Unix socket\n"
           " --include\n"
           "    Add the given path to the include path list. This option is\n"
           "    order-dependent, include paths given first are searched first.\n"
           "    If an include path is given, the default include path list is\n"
           "    not used. Use --include-defaults to add the default include\n"
           "    paths\n"
           " --include-defaults\n"
           "    Add the default set of include directories.\n"
           "    This option is order-dependent, include paths given first\n"
           "    are searched first.\n"
           "\n",
           argv[0]);
}

static bool
parse_options(int argc, char **argv, struct xkb_rule_names *names,
              char **socket_address)
{
    enum options {
        OPT_VERBOSE,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_SOCKET,
    };
    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"socket",           required_argument,      0, OPT_SOCKET},
        {0, 0, 0, 0},
    };

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(0);
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_SOCKET:
            *socket_address = optarg;
            break;
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        default:
            usage(argv);
            exit(EXIT_INVALID_USAGE);
        }
    }

    return true;
}

#define INPUT_BUFFER_SIZE 1024

static pthread_mutex_t server_state_mutext = PTHREAD_MUTEX_INITIALIZER;
static volatile int socket_fd;
static volatile bool serving = false;

static void
shutdown_server(void)
{
    pthread_mutex_lock(&server_state_mutext);
    fprintf(stderr, "Shuting down. Bye!\n");
    serving = false;
    shutdown(socket_fd, SHUT_RD);
    pthread_mutex_unlock(&server_state_mutext);
}

static void
handle_signal(int signum)
{
    switch (signum) {
        case SIGINT:
            shutdown_server();
            break;
    }
}

/* Load parser for a RMLVO component */
static bool
parse_component(char **input, size_t *max_len, const char **output)
{
    if (!(*input) || !(*max_len)) {
        *input = NULL;
        *max_len = 0;
        *output = NULL;
        return true;
    }
    char *start = *input;
    char *next = strchr(start, '\n');
    size_t len;
    if (next == NULL) {
        len = *max_len;
        *input = NULL;
        *max_len = 0;
    } else {
        len = (size_t)(next - start);
        *max_len -= len + 1;
        *input += len + 1;
    }
    *output = strndup(start, len);
    if (!(*output)) {
        fprintf(stderr, "ERROR: Cannot allocate memory\n");
        return false;
    }
    return true;
}

struct query_args {
    struct xkb_context *ctx;
    int accept_socket_fd;
};

static const char *
log_level_to_prefix(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_DEBUG:
        return "xkbcommon: DEBUG: ";
    case XKB_LOG_LEVEL_INFO:
        return "xkbcommon: INFO: ";
    case XKB_LOG_LEVEL_WARNING:
        return "xkbcommon: WARNING: ";
    case XKB_LOG_LEVEL_ERROR:
        return "xkbcommon: ERROR: ";
    case XKB_LOG_LEVEL_CRITICAL:
        return "xkbcommon: CRITICAL: ";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
keymap_log_fn(struct xkb_context *ctx, enum xkb_log_level level,
              const char *fmt, va_list args)
{
    const char *prefix = log_level_to_prefix(level);
    FILE *file = xkb_context_get_user_data(ctx);

    if (prefix)
        fprintf(file, "%s", prefix);
    vfprintf(file, fmt, args);
}

/* Process client’s queries */
static void*
process_query(void *x)
{
    struct query_args *args = x;
    int rc = EXIT_FAILURE;
    char input_buffer[INPUT_BUFFER_SIZE];

    /* Loop while client send queries */
    ssize_t count;
    while ((count = recv(args->accept_socket_fd,
                         input_buffer, INPUT_BUFFER_SIZE, 0)) > 0) {
        rc = EXIT_FAILURE;
        bool stop = true;
        if (input_buffer[0] == '\x1b') {
            /* Escape = exit */
            rc = -0x1b;
            break;
        }

        /*
         * Expected message load:
         * <1|0>\n<rules>\n<model>\n<layout>\nvariant>\n<options>
         */

        if (count < 3 || input_buffer[1] != '\n') {
            /* Invalid length */
            break;
        }
        bool serialize = input_buffer[0] == '1';

        /* We expect RMLVO to be provided with one component per line */
        char *input = input_buffer + 2;
        size_t len = count - 2;
        struct xkb_rule_names rmlvo = {
            .rules = NULL,
            .model = NULL,
            .layout = NULL,
            .variant = NULL,
            .options = NULL,
        };
        if (!parse_component(&input, &len, &rmlvo.rules) ||
            !parse_component(&input, &len, &rmlvo.model) ||
            !parse_component(&input, &len, &rmlvo.layout) ||
            !parse_component(&input, &len, &rmlvo.variant) ||
            !parse_component(&input, &len, &rmlvo.options)) {
            fprintf(stderr, "ERROR: Cannor parse RMLVO: %s.\n", input_buffer);
            goto error;
        }

        /* Response load: <length><keymap string> */

        /* Compile keymap */
        struct xkb_keymap *keymap;
        /* Clone context because it is not thread-safe */
        struct xkb_context ctx = *args->ctx;

        /* Set our own logging function to capture default stderr */
        char *stderr_buffer = NULL;
        size_t stderr_size = 0;
        FILE *stderr_new = open_memstream(&stderr_buffer, &stderr_size);
        if (!stderr_new) {
            perror("Failed to create in-memory stderr.");
            goto stderr_error;
        }
        xkb_context_set_user_data(&ctx, stderr_new);
        xkb_context_set_log_fn(&ctx, keymap_log_fn);

        rc = EXIT_SUCCESS;

        keymap = xkb_keymap_new_from_names(&ctx, &rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (keymap == NULL) {
            /* Send negative message length to convey error */
            count = -1;
            send(args->accept_socket_fd, &count, sizeof(count), MSG_NOSIGNAL);
            goto keymap_error;
        }

        /* Send the keymap, if required */
        if (serialize) {
            char *buf = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
            if (buf) {
                len = strlen(buf);
                /* Send message length */
                send(args->accept_socket_fd, &len, sizeof(len), MSG_NOSIGNAL);
                send(args->accept_socket_fd, buf, len, MSG_NOSIGNAL);
                free(buf);
            } else {
                count = -1;
                send(args->accept_socket_fd, &count, sizeof(count), MSG_NOSIGNAL);
            }
        } else {
            len = 0;
            send(args->accept_socket_fd, &len, sizeof(len), MSG_NOSIGNAL);
        }

        xkb_keymap_unref(keymap);

keymap_error:
        /* Wait that the client confirm the reception */
        recv(args->accept_socket_fd, input_buffer, 1, 0);

        /* Restore stderr for logging */
        fflush(stderr_new);
        xkb_context_set_user_data(&ctx, stderr);
        fclose(stderr_new);

        /* Send captured stderr */
        send(args->accept_socket_fd, &stderr_size, sizeof(stderr_size), MSG_NOSIGNAL);
        if (stderr_size && stderr_buffer) {
            send(args->accept_socket_fd, stderr_buffer, stderr_size, MSG_NOSIGNAL);
        }
        free(stderr_buffer);

        /* Wait that the client confirm the reception */
        input_buffer[0] = '0';
        recv(args->accept_socket_fd, input_buffer, 1, 0);
        stop = input_buffer[0] == '0';

stderr_error:
error:
        free((char*)rmlvo.rules);
        free((char*)rmlvo.model);
        free((char*)rmlvo.layout);
        free((char*)rmlvo.variant);
        free((char*)rmlvo.options);

        /* Close client connection if there was an error */
        if (rc != EXIT_SUCCESS || stop)
            break;
    }
    xkb_context_unref(args->ctx);
    close(args->accept_socket_fd);
    free(args);
    if (rc > 0) {
        fprintf(stderr, "ERROR: failed to process query. Code: %d\n", rc);
    } else if (rc < 0) {
        shutdown_server();
    }
    return NULL;
}

/* Create a server using Unix sockets */
static int
serve(struct xkb_context *ctx, const char* socket_address)
{
    int rc = EXIT_FAILURE;
    struct sockaddr_un sockaddr_un = { 0 };
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "ERROR: Cannot create Unix socket.\n");
        return EXIT_FAILURE;
    }
    /* Construct the bind address structure. */
    sockaddr_un.sun_family = AF_UNIX;
    strcpy(sockaddr_un.sun_path, socket_address);

    rc = bind(socket_fd, (struct sockaddr*) &sockaddr_un,
              sizeof(struct sockaddr_un));
    /* If socket_address exists on the filesystem, then bind will fail. */
    if (rc == -1) {
        fprintf(stderr, "ERROR: Cannot create Unix socket path.\n");
        rc = EXIT_FAILURE;
        goto error_bind;
    };
    if (listen(socket_fd, 4096) == -1) {
        fprintf(stderr, "ERROR: Cannot listen socket.\n");
        goto error;
    }
    signal(SIGINT, handle_signal);
    fprintf(stderr, "Serving...\n");
    serving = true;

    struct timeval timeout = {
        .tv_sec = 3,
        .tv_usec = 0
    };

    while (1) {
        int accept_socket_fd = accept(socket_fd, NULL, NULL);
        if (accept_socket_fd == -1) {
            if (serving) {
                fprintf(stderr, "ERROR: fail to accept query\n");
                rc = EXIT_FAILURE;
            } else {
                rc = EXIT_SUCCESS;
            }
            goto error;
        };

        if (accept_socket_fd > 0) {
            /* Client is connected */
            pthread_t thread;

            /* Prepare worker’s context */
            struct query_args *args = calloc(1, sizeof(struct query_args));
            if (!args) {
                close(accept_socket_fd);
                continue;
            }
            /* Context will be cloned in worker */
            args->ctx = xkb_context_ref(ctx);
            args->accept_socket_fd = accept_socket_fd;

            if (setsockopt(accept_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                           sizeof timeout) < 0)
                perror("setsockopt failed\n");

            /* Launch worker */
            rc = pthread_create(&thread, NULL, process_query, args);
            if (rc) {
                perror("Error creating thread: ");
                close(accept_socket_fd);
                free(args);
                continue;
            }
            pthread_detach(thread);
        }
    }
error:
    close(socket_fd);
    unlink(socket_address);
error_bind:
    fprintf(stderr, "Exiting...\n");
    return rc;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    struct xkb_rule_names names = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };
    char *socket_address = NULL;
    int rc = EXIT_FAILURE;

    if (argc < 1) {
        usage(argv);
        return EXIT_INVALID_USAGE;
    }

    if (!parse_options(argc, argv, &names, &socket_address))
        return EXIT_INVALID_USAGE;

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    assert(ctx);

    if (verbose) {
        xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
        xkb_context_set_log_verbosity(ctx, 10);
    }

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

#ifdef ENABLE_KEYMAP_CACHE
    struct xkb_keymap_cache *keymap_cache = xkb_keymap_cache_new();
    if (keymap_cache)
        ctx->keymap_cache = keymap_cache;
    else
        fprintf(stderr, "ERROR: Cannot create keymap cache.\n");
#endif
    serve(ctx, socket_address);
#ifdef ENABLE_KEYMAP_CACHE
    xkb_keymap_cache_free(keymap_cache);
#endif

    xkb_context_unref(ctx);

    return rc;
}
