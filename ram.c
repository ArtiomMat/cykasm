#include <stdio.h>
#include <sys/mman.h>

#define ALIGN (sizeof(long long))

typedef struct
{
  unsigned n;
  unsigned fl;
  unsigned location;
} head;

static unsigned mem_n = 0;
static unsigned mem_fn = 0; // Free number of bytes
static void* mem = 0;

int ram_on(unsigned _n)
{
  mem_n = _n;
  mem = mmap(0, mem_n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  
  if (mem == MAP_FAILED)
  {
    return 0;
  }

  mem_fn = mem_n;
  return 1;
}

int ram_off()
{
  return munmap(mem, mem_n) != -1;
}

/**
 * Aligned to 8 bytes.
 * Returns 0 on failure.
 */
void* ram_mark(unsigned n, char fixed_size)
{
  if (mem_fn < n)
  {
    return 0;
  }

  
}

void ram_free(void* mark)
{

}

int main()
{
  ram_on(512);
  return 0;
}
