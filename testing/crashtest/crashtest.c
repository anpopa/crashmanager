/*
 * SPDX license identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2019-2020 Alin Popa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * \author Alin Popa <alin.popa@fxdata.ro>
 * \file crashtest.c
 */

#include <cdh-epilog.h>

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TEST_BUFFER_MB(x) (sizeof (uint8_t) * x * 1024 * 1024)
#define CRASH_MSG_LEN 1024

typedef enum _crashtype
{
  crash_abrt,
  crash_segv1,
  crash_segv2
} crashtype;

static void
on_crash_cb (int efd, int signum)
{
  char msg[CRASH_MSG_LEN] = {};
  ssize_t sz;

  sz = snprintf (msg, CRASH_MSG_LEN, "Crashed with signal '%s' and is sad!\n", strsignal (signum));
  if (sz < 0 || sz >= CRASH_MSG_LEN)
    return;

  sz = write (efd, msg, sz + 1);
  if (sz < 0)
    return;
}

static uint8_t *
allocate_buffer (size_t sz, bool rdz)
{
  uint8_t *buf = (uint8_t *)malloc (TEST_BUFFER_MB (sz));
  uint8_t val = 0;

  assert (buf);

  for (size_t i = 0; i < TEST_BUFFER_MB (sz); i++)
    {
      if (rdz == true)
        buf[i] = (uint8_t)(rand () & 0xFF);
      else
        buf[i] = val++;
    }

  for (size_t i = 0; i < TEST_BUFFER_MB (sz); i++)
    (void)buf[i];

  return buf;
}

int
main (int argc, char *argv[])
{
  int c;
  bool help = false;
  int long_index = 0;
  size_t size = 0;
  uint8_t *test_buffer = NULL;
  bool randomize = false;
  crashtype type = crash_abrt;

  struct option longopts[] = { { "type", required_argument, NULL, 't' },
                               { "size", required_argument, NULL, 's' },
                               { "rand", no_argument, NULL, 'r' },
                               { "help", no_argument, NULL, 'h' },
                               { NULL, 0, NULL, 0 } };

  while ((c = getopt_long (argc, argv, "t:s:r::h", longopts, &long_index)) != -1)
    switch (c)
      {
      case 't':
        type = (crashtype)strtol (optarg, NULL, 10);
        break;

      case 's':
        size = (size_t)strtol (optarg, NULL, 10);
        break;

      case 'r':
        randomize = true;
        break;

      case 'h':
        help = true;
        break;

      default:
        break;
      }

  if (help)
    {
      printf ("crashtest: simulate a crash at specific location\n\n");
      printf ("Usage: crashtest [OPTIONS] \n\n");
      printf ("  General:\n");
      printf ("     --type, -t  <number>  0 - fixed ABRT, 1 - SEGV pos1 2 - SEGV pos2 \n");
      printf ("     --size, -s  <number>  Coredump size to simulate in MB \n");
      printf ("     --rand, -r            Randomize allocated memory \n");
      printf ("  Help:\n");
      printf ("     --help, -h            Print this help\n\n");
      exit (EXIT_SUCCESS);
    }

  cdh_epilog_register_crash_handlers (on_crash_cb);

  if (randomize == true)
    srand ((unsigned int)time (0));

  if (size > 0)
    test_buffer = allocate_buffer (size, randomize);

  if (type == crash_abrt)
    goto crashpos0;

  if (type == crash_segv1)
    goto crashpos1;

  if (type == crash_segv2)
    goto crashpos2;

  if (test_buffer)
    free (test_buffer);

crashpos0:
  printf ("Simulate abort at line %d\n", __LINE__);
  abort ();

crashpos1:
  printf ("Simulate segv at line %d\n", __LINE__);
  *(int *)0 = 1;

crashpos2:
  printf ("Simulate segv at line %d\n", __LINE__);
  *(int *)0 = 2;

  exit (EXIT_SUCCESS);
}
