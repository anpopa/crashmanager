/* crashtest.c
 *
 * Copyright 2019 Alin Popa <alin.popa@bmw.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
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
