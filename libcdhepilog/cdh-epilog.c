/*
 * SPDX license identifier: MIT
 *
 * Copyright (c) 2020 Alin Popa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file cdh-epilog.c
 */

#include "cdh-epilog.h"
#include "cdh-elogmsg.h"

#include <execinfo.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef CDH_EPILOG_BUF_SIZE
#define CDH_EPILOG_BUF_SIZE (4096)
#endif

#ifndef CDH_EPILOG_SOCKET_TIMEOUT_SEC
#define CDH_EPILOG_SOCKET_TIMEOUT_SEC (5)
#endif

static const char *elog_socket_path = "/run/crashmanager/.epilog.sock";
static const char *backtrace_marker = "[backtrace]\n";
static const char *userdata_marker = "\n[userdata]\n";

static pthread_mutex_t _epilog_mutex = PTHREAD_MUTEX_INITIALIZER;
static cdh_epilog_oncrash_callback _oncrash_callback = NULL;
static bool _epilog_handler_executed = false;

static inline void epilog_unregister_signal_handler(void);
static inline void epilog_register_signal_handler(void);
static void epilog_signal_handler(int signum);

static inline void epilog_register_signal_handler()
{
    signal(SIGFPE, epilog_signal_handler);
    signal(SIGILL, epilog_signal_handler);
    signal(SIGIOT, epilog_signal_handler);
    signal(SIGBUS, epilog_signal_handler);
    signal(SIGSYS, epilog_signal_handler);
    signal(SIGTRAP, epilog_signal_handler);
    signal(SIGXCPU, epilog_signal_handler);
    signal(SIGXFSZ, epilog_signal_handler);
    signal(SIGQUIT, epilog_signal_handler);
    signal(SIGABRT, epilog_signal_handler);
    signal(SIGSEGV, epilog_signal_handler);
}

static inline void epilog_unregister_signal_handler()
{
    signal(SIGFPE, NULL);
    signal(SIGILL, NULL);
    signal(SIGIOT, NULL);
    signal(SIGBUS, NULL);
    signal(SIGSYS, NULL);
    signal(SIGTRAP, NULL);
    signal(SIGXCPU, NULL);
    signal(SIGXFSZ, NULL);
    signal(SIGQUIT, NULL);
    signal(SIGABRT, NULL);
    signal(SIGSEGV, NULL);
}

static void epilog_signal_handler(int signum)
{
    void *buffer[CDH_EPILOG_BUF_SIZE] = {};
    CdmELogMessage *msg = NULL;
    const char *spath = NULL;
    int nptrs = -1;
    ssize_t sz = 0;
    int fd = -1;

    struct timeval tout = {.tv_sec = CDH_EPILOG_SOCKET_TIMEOUT_SEC, .tv_usec = 0};

    struct sockaddr_un saddr = {.sun_family = AF_UNIX, .sun_path = {}};

    /* we should not execute the epilog signal handler again */
    if (pthread_mutex_trylock(&_epilog_mutex) != 0)
        return;

    if (_epilog_handler_executed) {
        pthread_mutex_unlock(&_epilog_mutex);
        return;
    }

    _epilog_handler_executed = true;

    /* initiate communication with crashmanager */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        goto epilog_handler_exit;

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &tout, sizeof(tout)) == -1)
        goto epilog_handler_exit;

    if ((nptrs = backtrace(buffer, CDH_EPILOG_BUF_SIZE)) == -1)
        goto epilog_handler_exit;

    /* set the socket path */
    spath = getenv("EPILOG_SOCK") != NULL ? getenv("EPILOG_SOCK") : elog_socket_path;
    snprintf(saddr.sun_path, (sizeof(saddr.sun_path) - 1), "%s", spath);

    if (connect(fd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_un)) == -1)
        goto epilog_handler_exit;

    msg = cdm_elogmsg_new(CDM_ELOGMSG_NEW);

    cdm_elogmsg_set_process_pid(msg, getpid());
    cdm_elogmsg_set_process_exit_signal(msg, signum);

    if (cdm_elogmsg_write(fd, msg) != 0)
        goto epilog_handler_exit;

    /* send the backtrace */
    sz = write(fd, backtrace_marker, strlen(backtrace_marker));
    if (sz <= 0)
        goto epilog_handler_exit;

    backtrace_symbols_fd(buffer, nptrs, fd);

    /* call the user data handler */
    if (_oncrash_callback != NULL) {
        sz = write(fd, userdata_marker, strlen(userdata_marker));
        if (sz <= 0)
            goto epilog_handler_exit;

        _oncrash_callback(fd, signum);
    }

epilog_handler_exit:
    if (fd != -1)
        close(fd);

    if (msg != NULL)
        cdm_elogmsg_free(msg);

    /*
     * Reset all signal handlers now potentially after user _oncrash_callback ()
     * to avoid the case when user register other signal handlers in their callback.
     * CrashManager expects our crash and core dump to attach this epilog.
     */
    epilog_unregister_signal_handler();

    pthread_mutex_unlock(&_epilog_mutex);
    raise(signum); /* raise again the same signal to exit */
}

void cdh_epilog_register_crash_handlers(cdh_epilog_oncrash_callback callback)
{
    _oncrash_callback = callback;
    epilog_register_signal_handler();
}
