/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "test.h"
#include "xvfb-wrapper.h"

static volatile bool xvfb_is_ready;

static void
sigusr1_handler(int signal)
{
    xvfb_is_ready = true;
}

int
xvfb_wrapper(x11_test_func_t test_func, void *private)
{
    int ret = EXIT_SUCCESS;
    FILE * display_fd;
    char display_fd_string[32];
    sigset_t mask;
    struct sigaction sa;
    char *xvfb_argv[] = {
        (char *) "Xvfb", (char *) "-displayfd", display_fd_string, NULL
    };
    char *envp[] = { NULL };
    pid_t xvfb_pid = 0;
    size_t counter = 0;
    char display[32] = ":";
    size_t length;

    /* File descriptor to retrieve the display number */
    display_fd = tmpfile();
    if (display_fd == NULL){
        fprintf(stderr, "Unable to create temporary file.\n");
        goto err_display_fd;
    }
    snprintf(display_fd_string, sizeof(display_fd_string), "%d", fileno(display_fd));

    /* Set SIGUSR1 to SIG_IGN so Xvfb will send us that signal when it's ready
     * to accept connections. In order to avoid race condition, we block the
     * until we are ready to process it. */
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    struct sigaction sa_old;
    sigaction(SIGUSR1, &sa, &sa_old);

    xvfb_is_ready = false;

    /*
     * Xvfb command: let the server find an available display.
     *
     * Note that it may generate multiple times the following output in stderr:
     *    _XSERVTransSocketUNIXCreateListener: ...SocketCreateListener() failed
     * It is expected: this is the server trying the ports until it finds one
     * that works.
     */
    ret = posix_spawnp(&xvfb_pid, "Xvfb", NULL, NULL, xvfb_argv, envp);
    if (ret != 0) {
        fprintf(stderr,
                "[ERROR] Cannot run Xvfb. posix_spawnp error %d: %s\n",
                ret, strerror(ret));
        if (ret == ENOENT) {
            fprintf(stderr,
                    "[ERROR] Xvfb may be missing. "
                    "Please install the corresponding package, "
                    "e.g. \"xvfb\" or \"xorg-x11-server-Xvfb\".\n");
        }
        ret = TEST_SETUP_FAILURE;
        goto err_xvfd;
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    /* Now wait for the SIGUSR1 signal that Xvfb is ready */
    while (!xvfb_is_ready) {
        usleep(1000);
        if (++counter >= 3000) /* 3 seconds max wait */
            break;
    }

    sigaction(SIGUSR1, &sa_old, NULL);

    /* Check if Xvfb is still alive */
    pid_t pid = waitpid(xvfb_pid, NULL, WNOHANG);
    if (pid != 0) {
        fprintf(stderr, "ERROR: Xvfb not alive\n");
        ret = TEST_SETUP_FAILURE;
        goto err_xvfd;
    }

    /* Retrieve the display number: Xvfd writes the display number as a newline-
     * terminated string; copy this number to form a proper display string. */
    fseek(display_fd, 0, SEEK_SET);
    length = fread(&display[1], 1, sizeof(display) - 2, display_fd);
    if (length <= 0) {
        fprintf(stderr, "fread error: length=%zu\n", length);
        ret = TEST_SETUP_FAILURE;
        goto err_xvfd;
    } else {
        /* Drop the newline character */
        display[length] = '\0';
    }

    /* Run the function requiring a running X server.
     * Because it may call abort() via assert(), we fork to be able to
     * exit gracefully and not hang waiting for Xvfb. */
    pid_t test_pid = fork();
    switch (test_pid) {
        case -1:
            perror("fork");
            ret = TEST_SETUP_FAILURE;
            break;
        case 0:
            fprintf(stderr, "Running test using Xvfb wrapper...\n");
            ret = test_func(display, private);
            fprintf(stderr,
                    "Test using Xvfb wrapper finished with code %d.\n", ret);
            _exit(ret);
        default:
            {
                int test_status = 0;
                pid_t test_pid2 = waitpid(test_pid, &test_status, 0);
                ret = (test_pid2 > 0 && WIFEXITED(test_status))
                    ? WEXITSTATUS(test_status)
                    : EXIT_FAILURE;
                fprintf(stderr,
                        "Test finished with code %d. "
                        "Shutting down Xvfb (pid: %d)...\n",
                        ret, xvfb_pid);
            }
    }

err_xvfd:
    if (xvfb_pid > 0) {
        fprintf(stderr, "Sending SIGTERM to Xvfb...\n");
        kill(xvfb_pid, SIGTERM);
        fprintf(stderr, "Waiting for Xvfb to exit (pid: %d)...\n", xvfb_pid);
        int xvfb_status = 0;
        if (waitpid(xvfb_pid, &xvfb_status, 0) <= 0) {
            perror("Xvfb waitpid failed.");
        } else if (WIFEXITED(xvfb_status)) {
            fprintf(stderr, "Xvfb shut down (pid: %d) with exit code %d.\n",
                    xvfb_pid, WEXITSTATUS(xvfb_status));
        } else {
            fprintf(stderr, "Xvfb shut down (pid: %d) abnormally.\n", xvfb_pid);
        }
    }
    fclose(display_fd);
err_display_fd:
    return ret;
}

/* All X11_TEST functions are in the test_func_sec ELF section.
 * __start and __stop point to the start and end of that section. See the
 * __attribute__(section) documentation.
 */
DECLARE_TEST_ELF_SECTION_POINTERS(TEST_ELF_SECTION);

int
x11_tests_run(void)
{
    int rc = 0;
    for (const struct test_function *t = __start_test_func_sec;
         t < __stop_test_func_sec;
         t++) {
        fprintf(stderr, "------ Running test: %s from %s ------\n",
                t->name, t->file);
        rc = xvfb_wrapper(t->func, NULL);
        if (rc != EXIT_SUCCESS) {
            break;
        }
    }

    return rc;
}
