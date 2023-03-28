#include <hedley.h>
#include <stdarg.h>
#include <stdio.h>

#include "attrs.h"
#include "global_settings.h"
#include "util.h"

NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(1, 2)
void log_verbose(const char *restrict fmt, ...) {
  if (global_settings.verbosity > VERBOSE_SOME) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}

NON_NULL_PARAMS
HEDLEY_PRINTF_FORMAT(1, 2)
void log_extra_verbose(const char *restrict fmt, ...) {
  if (global_settings.verbosity > VERBOSE_VERY) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}
