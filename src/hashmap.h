typedef struct {
  u32 n_buckets;
  u32 n_elems;
  void *data;
} a_hashmap;

typedef int (*comparator)(const void *, const void *, void *);
typedef int (*hasher)(const void *, void *);

a_hashmap ahm_new(comparator c, hasher h, void *context);

void ahm_insert(void *data);
