/*
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2019, BMW Car IT GmbH
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * \author Alin Popa <alin.popa@bmw.de>
 * \file crashtest.c
 */

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TEST_BUFFER_MB(x) (sizeof(uint8_t) * x * 1024 * 1024)

typedef enum _crashtype { crash_abrt, crash_segv1, crash_segv2 } crashtype;

static uint8_t *
allocate_buffer (size_t sz, bool rdz)
{
  uint8_t *buf = (uint8_t*)malloc (TEST_BUFFER_MB (sz));
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
  *(int*)0 = 1;

crashpos2:
  printf ("Simulate segv at line %d\n", __LINE__);
  *(int*)0 = 2;

  exit (EXIT_SUCCESS);
}
