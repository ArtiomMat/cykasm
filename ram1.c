#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef __x86_64
  #error RAM only works on 64 bit machines.
#endif

// Says `rami`
#define SIGNATURE 0x72616d69

typedef struct
{
  int signature; // Magical characters that define if it's on
  unsigned size; // Size of buffer, not including the ram_head
} head_t;

/**
 * On success, returns a pointer to a buffer with `n` elements of size `size`.
 * On failure, returns `(void*) 0`.
*/
void* ram_want(unsigned size)
{
  void* p = mmap(0, size + sizeof(head_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  
  if (p == MAP_FAILED)
  {
    return (void*) 0;
  }

  head_t* head = p;
  head->size = size;
  head->signature = SIGNATURE;

  return p + sizeof(head_t);
}

/**
 * If the pointer is a valid `ram` pointer, returns 1 after freeing it.
 * If the pointer
*/
int ram_free(void* p)
{
  if (!p)
    return 0;
  
  head_t* head = p - sizeof(head_t);
  
  if (head->signature != SIGNATURE)
    return 0;
  
  head->signature = 0; // Destroy the signature

  munmap(head, head->size);
  return 1;
}

int main()
{
  
}
