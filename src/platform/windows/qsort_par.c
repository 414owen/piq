#include <string.h>

/*
void qsort_s(
   void *base,
   size_t num,
   size_t width,
   int (__cdecl *compare )(void *, const void *, const void *),
   void * context
);
*/

typedef int (*cmp)(const void *, const void *, void *);

typedef struct {
  cmp f;
  void *context;
} qsort_data_wrapper;

int cmp_wrapper(void *data, const void *a, const void *b) {
  qsort_data_wrapper *w = (qsort_data_wrapper*) data;
  return w->f(a, b, w->context);
}

void qsort_r(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *, void *),
           void *arg) {
  qsort_data_wrapper data = {
    .f = compar,
    .context = arg,
  };
  qsort_s(base, nmemb, size, cmp_wrapper, &data);
}
