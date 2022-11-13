#include <predef/predef.h>

#ifdef PREDEF_LIBC_GNU
  #define _GNU_SOURCE
  #include <string.h>
#else

#include <string.h>

// TODO test
void *memmem(const void *restrict haystack, size_t haystack_len, const void *restrict needle, size_t needle_len) {
	const char *hs = (const char *)haystack;
	const char *nd = (const char *)needle;
	if
    (  haystack == NULL
    || haystack == NULL
    || needle_len > haystack_len
    || haystack_len == 0
    || needle_len == 0) {
		return NULL;
  }

	if (needle_len == 1) {
		return memchr(hs, (int) nd[0], haystack_len);
  }

	const char *last = hs + haystack_len - needle_len;

	for (char *pos = (char *)haystack; pos <= last; pos++) {
		if (memcmp(pos, nd, needle_len) == 0) {
			return pos;
    }
  }

	return NULL;
}

#endif
