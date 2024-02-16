#pragma once

#ifndef __x86_64
  #error RAM only works on 64 bit machines.
#endif

typedef unsigned long long u64_t;

typedef struct
{
  /**
   * An index stores all the key-value-pairs for which the key has the same hash.
  */
  struct __hash_row
  {
    int n; // Number of key-value-pairs
    int mem_n; // Number of key-value-pairs in memory
    struct __hash_pair
    {
      u64_t key, value;
      struct __hash_pair* next;
    };
  }* indices;
  // Number of indices
  int n;
} hash_t;

typedef struct
{
  
} list;

typedef struct
{
  char* c;
} zstr;

typedef struct
{

} buff;

/**
 * `n` is the number of key-value-pairs estimated to be in the hash, underestimation will not cause "problems", but may cause noticable performance decrease if too low due to distinction duplicate entry keys. 0 sets it to a generalized value that aims for low memory usage.
*/
void
init_hash(hash_t* hash, int ks, int vs, int n);
void
free_hash(hash_t* hash);
/**
 * Pushes a key-value-pair.
 * NOTE: Must adhere to sizes set in init_hash
*/
void
push_hash(hash_t* hash, void* k, void* v);
/**
 * Peeks at a value using a key, does not remove it.
*/
void*
peek_hash(hash_t* hash, void* k);
/**
 * Removes a value from a hash_t.
*/
void
pull_hash(hash_t* hash, char* s);


