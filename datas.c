#include "datas.h"
#include <stdlib.h>

unsigned
hash8(unsigned long long x)
{
  return x ^ (x>>15) * (x>>23) ^ (x>>31) * (x>>39);
}

void
init_hash(hash_t* hash, int ks, int vs, int n)
{
  // hash->ks = ks;
  hash->n = n;
  // hash->vs = vs;
  hash->indices = calloc(n, sizeof(*hash->indices)); // To NULL pointer the pointers :)
  for (int i = 0; i < n; i++)
  {
    hash->indices->mem_n = 1;
    // hash->indices.
  }
}
void
free_hash(hash_t* hash)
{
  free(hash->indices);
}
void
push_hash(hash_t* hash, void* k, void* v)
{
  unsigned ii = hash8(*(unsigned long long*)k) % hash->n;
  // if (hash->indices[ii].n )
}
void* peek_hash(hash_t* hash, void* k);
void pull_hash(hash_t* hash, char* s);

int
main()
{
  int a[20] = {0};
  for (int i = 0; i < 20; i++)
  {
    unsigned long long x = rand() + (((unsigned long long)rand())<<31);
    a[hash8(x)%(sizeof(a)/sizeof(a[0]))]++;
  }

  for (int i = 0; i < sizeof(a)/sizeof(a[0]); i++)
  {
    printf("%i has %i entries\n", i, a[i]);
  }

  return 0;
}
