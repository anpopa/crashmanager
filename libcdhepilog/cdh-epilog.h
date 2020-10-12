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
 * \file cdh-epilog.h
 */

/*
 * @brief User callback function on crash. The application can do on crash cleanup
 *        and write additional data into epilog file descriptor, but should not raise
 *        new signals.
 * @param efd The open file descriptor the user can write data into
 * @param signum The crash signal causing the exit
 */
typedef void            (*cdh_epilog_oncrash_callback)      (int efd, int signum);

/*
 * @brief Register internal signal handler for all core generating signals to send
 *        epilog to crashmanager before crash.
 * @param callback The callback function to call in crash handler in order to allow
 *        user to dump additional information. Can be NULL.
 */

extern void             cdh_epilog_register_crash_handlers  (cdh_epilog_oncrash_callback callback);
