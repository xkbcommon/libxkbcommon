/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <spawn.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include "utils.h"
#include "darray.h"

#define PIPE_WRITE 1
#define PIPE_READ 0
#define SETUP_FAILURE 99

int
compile_with(const char* compiler_name, const char* const *compiler_argv,
             const char *keymap_in, size_t keymap_size_in,
             char **keymap_out, size_t *keymap_size_out);

int
compile_with(const char* compiler_name, const char* const *compiler_argv,
             const char *keymap_in, size_t keymap_size_in,
             char **keymap_out, size_t *keymap_size_out)
{
    int ret = EXIT_FAILURE;
    assert(keymap_in);

    /* Prepare parameters */
    char *envp[] = { NULL };

    /* Prepare input */
    int stdin_pipe[2] = {0};
    int stdout_pipe[2] = {0};
    posix_spawn_file_actions_t action;
    if (pipe(stdin_pipe) == -1) {
        perror("stdin pipe");
        ret = SETUP_FAILURE;
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

    /* Prepare stdout */
    if (pipe(stdout_pipe) == -1) {
        perror("stdout pipe");
        ret = SETUP_FAILURE;
        goto pipe_error;
    }

    if (posix_spawn_file_actions_addclose(&action, stdout_pipe[PIPE_READ]))
        goto posix_spawn_file_actions_error;
    if (posix_spawn_file_actions_adddup2(&action, stdout_pipe[PIPE_WRITE], STDOUT_FILENO))
        goto posix_spawn_file_actions_error;

    /* Launch compiler */
    pid_t compiler_pid;
    ret = posix_spawnp(&compiler_pid, compiler_name, &action, NULL,
                       (char* const*)compiler_argv, envp);
    if (ret != 0) {
        errno = ret;
        perror("posix_spawnp XKB compiler failed");
        goto posix_spawn_file_actions_init_error;
    }
    /* Close unused read-end of pipe */
    close(stdin_pipe[PIPE_READ]);
    close(stdout_pipe[PIPE_WRITE]);
    ssize_t ret2 = write(stdin_pipe[PIPE_WRITE], keymap_in, keymap_size_in);
    /* Close write-end of the pipe, to emit EOF */
    close(stdin_pipe[PIPE_WRITE]);
    if (ret2 == -1) {
        perror("Cannot write keymap to stdin");
        kill(compiler_pid, SIGTERM);
    }

    /* Capture stdout */
    const size_t buffer_size = 1024;
    darray_char stdout = darray_new();
    darray_resize0(stdout, buffer_size);
    size_t size = 0;
    ssize_t count;
    while ((count = read(stdout_pipe[PIPE_READ], darray_items(stdout) + size,
                         buffer_size)) > 0) {
        size += (size_t) count;
        darray_resize0(stdout, size + buffer_size);
    }
    close(stdout_pipe[PIPE_READ]);

    /* Ensure we get a NULL-terminated string */
    if (size > 0 && darray_item(stdout, size - 1) != '\0')
        darray_resize0(stdout, size + 1);

    darray_steal(stdout, keymap_out, NULL);
    *keymap_size_out = size;

    /* Wait for compiler to complete */
    int status;
    ret = waitpid(compiler_pid, &status, 0);
    ret = (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        ? SETUP_FAILURE
        : EXIT_SUCCESS;
    goto cleanup;

posix_spawn_file_actions_error:
    perror("posix_spawn_file_actions_* error");
posix_spawn_file_actions_init_error:
    close(stdin_pipe[PIPE_WRITE]);
    close(stdin_pipe[PIPE_READ]);
    close(stdout_pipe[PIPE_READ]);
    close(stdout_pipe[PIPE_WRITE]);
    ret = SETUP_FAILURE;
cleanup:
    posix_spawn_file_actions_destroy(&action);
pipe_error:
    return ret;
}

int
compile_with_xkbcomp(const char* display, const char *include_path,
                     const char *keymap_in, size_t keymap_size_in,
                     char **keymap_out, size_t *keymap_size_out);

int
compile_with_xkbcomp(const char* display, const char *include_path,
                     const char *keymap_in, size_t keymap_size_in,
                     char **keymap_out, size_t *keymap_size_out)
{
    const bool has_display = !isempty(display);
    const char* const out = (has_display) ? display : "-";

    /* Prepare xkbcomp parameters */
    char include_path_arg[PATH_MAX+2] = "-I";
    const char *xkbcomp_argv[] = {
        "xkbcomp", "-I" /* reset include path*/, include_path_arg,
        "-opt", "g", "-w", "10", "-xkb", "-" /* stdin */, out, NULL
    };
    if (include_path) {
        int ret = snprintf(include_path_arg, ARRAY_SIZE(include_path_arg),
                           "-I%s", include_path);
        if (ret < 0 || ret >= (int)ARRAY_SIZE(include_path_arg)) {
            return SETUP_FAILURE;
        }
    }

    return compile_with("xkbcomp", xkbcomp_argv,
                        keymap_in, keymap_size_in,
                        keymap_out, keymap_size_out);
}

int
compile_with_kbvm(const char *include_path,
                  const char *keymap_in, size_t keymap_size_in,
                  char **keymap_out, size_t *keymap_size_out);

int
compile_with_kbvm(const char *include_path,
                  const char *keymap_in, size_t keymap_size_in,
                  char **keymap_out, size_t *keymap_size_out)
{
    /* Prepare xkbcomp parameters */
    const char *kbvm_argv[] = {
        "kbvm", "compile-xkb", "-", NULL, NULL, NULL, NULL
    };
    if (include_path) {
        kbvm_argv[3] = "--no-default-includes";
        kbvm_argv[4] = "--append-include";
        kbvm_argv[5] = include_path;
    }

    return compile_with("kbvm", kbvm_argv,
                        keymap_in, keymap_size_in,
                        keymap_out, keymap_size_out);
}
