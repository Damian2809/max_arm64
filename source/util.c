#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "config.h"

#ifdef DEBUG_LOG

void userAppInit(void) {
  // I SHOULD DO SOMETHING HERE BUT I DONT CARE
}

void userAppExit(void) {
  // FUCK YOU, IDK IF I NEED YOU
}

#endif

int debugPrintf(const char *text, ...) {
#ifdef DEBUG_LOG
  va_list list;

  // Log to a file, that is really stupid to do and will probably fuck up everything
  FILE *f = fopen(LOG_NAME, "a");
  if (f) {
    va_start(list, text);
    vfprintf(f, text, list);
    va_end(list);
    fclose(f);
  }

  // Log to the console
  va_start(list, text);
  vprintf(text, list);
  va_end(list);
#endif
  return 0;
}

int ret0(void) { return 0; }

int ret1(void) { return 1; }

int retm1(void) { return -1; }