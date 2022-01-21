#include <stdlib.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define STATIC_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

void *memclone(void *src, size_t bytes);
char *join(size_t str_amt, char **strs);
