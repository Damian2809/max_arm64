/* error.c -- error handler
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h> // For sleep()

#include "util.h"
#include "error.h"

void fatal_error(const char *fmt, ...) {
  // Initialize console (no-op on Linux, as stdout/stderr are already connected to the terminal)
  printf("\x1B[31m"); // Set text color to red for error message
  printf("FATAL ERROR:\n");

  // Print the error message
  va_list list;
  va_start(list, fmt);
  vprintf(fmt, list);
  va_end(list);

  printf("\n\nPress ENTER to exit.\n");
  printf("\x1B[0m"); // Reset text color

  // Wait for user input (ENTER key)
  char buf[2];
  fgets(buf, sizeof(buf), stdin);

  // Exit the program
  exit(1);
}