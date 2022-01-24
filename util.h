#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define STATIC_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef struct {
  char *string;
  FILE *stream;
} stringstream;

void ss_init(stringstream *ss);
void ss_finalize(stringstream *ss);

void *memclone(void *src, size_t bytes);
char *join(size_t str_amt, char **strs);
int vasprintf(char **, const char * restrict, va_list);
int asprintf(char **, const char *restrict, ...);
