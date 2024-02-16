#include <stdio.h>
#include <stdlib.h>

enum
{
  CELL_CLEAR='.', // Clear area
  CELL_WATER='~', // Not actually water, just area that can't cross
  CELL_START='i', // Start position
  CELL_FINAL='f', // Final position
  CELL_ENEMY='!', // Enemies
  CELL_TREES='t', // Forest, trees and such.
  CELL_HOUSE='h', // A house/building
  CELL_RISKY='?', // Risky area, for instance land mines
};

// Cells are considered to be approx. 30 meters.
typedef struct
{
  char* cells;
  int* ascores;
  int w, h;
  int sx, sy, fx, fy; // Stored the points where we start and end
} map_t;

map_t map = {0};

char get_cell(int x, int y)
{
  return map.cells[y*map.w+x];
}
// DOes bound checking and returns __INT_MAX__ if out of bounds
int get_as(int x, int y)
{
  if (x >= map.w || x < 0 || y >= map.h || y < 0)
  {
    return __INT_MAX__;
  }
  return map.ascores[y*map.w+x];
}

void i_to_xy(int i, int* x , int* y)
{
  *x = i % map.w;
  *y = i / map.w;
}

void print_map()
{
  if (!map.cells)
  {
    return;
  }

  int i = 0;
  for (int y = 0; y < map.h; y++)
  {
    for (int x = 0; x < map.w; x++)
    {
      putc(map.cells[i++], stdout);
    }
    putc('\n', stdout);
  }
}

// Anti-score is just score but anti, the lower the better.
// Returns anti-score of a cell based on position of final x and y.
int ascore(int x, int y, int fx, int fy)
{
  if (get_cell(x,y) == CELL_ENEMY)
  {
    return __INT_MAX__;
  }
  
  int dx = x-fx;
  dx = dx<0?-dx:dx;

  int dy = y-fy;
  dy = dy<0?-dy:dy;

  return dx + dy;
}

int main(int args_n, char** args)
{
  if (args_n < 2)
  {
    printf("Usage: %s <map.map file>", args[0]);
    return 1;
  }
  
  FILE* mapf = fopen(args[1], "r");

  map.w = -1;
  map.h = 1;
  {
    int w = 0; // Temorary w for validating that it's constant
    // Initial read to determine dimentions
    for (int c = fgetc(mapf); c != EOF; c = fgetc(mapf))
    {
      if (c == '\n')
      {
        if (map.w == -1) // First time writing
        {
          map.w = w;
        }
        else if (map.w != w)
        {
          printf("Map width is not constant, line %i.", map.h);
          return 1;
        }

        map.h++;
        w = 0;
      }
      else
      {
        w++;
      }
    }
  }

  map.cells = malloc(sizeof (char) * map.w * map.h);


  // Now we copy it! + set the sx/y and fx/y
  rewind(mapf);
  for (int c = fgetc(mapf), i = 0; c != EOF; c = fgetc(mapf), i++)
  {
    if (c != '\n')
    {
      if (c == CELL_START)
      {
        i_to_xy(i, &map.sx, &map.sy);
      }
      else if (c == CELL_FINAL)
      {
        i_to_xy(i, &map.fx, &map.fy);
      }
      map.cells[i] = c;
    }
    else // Because it still increments!
    {
      i--;
    }
  }

  // Set up the ascores
  map.ascores = malloc(sizeof (int) * map.w * map.h);
  int i = 0;
  for (int y = 0; y < map.h; y++)
  {
    for (int x = 0; x < map.w; x++, i++)
    {
      map.ascores[i] = ascore(x, y, map.fx, map.fy);
    }
  }

  // Now just calculate the path!
  int x = map.sx, y = map.sy;
  int as = -1;
  while (get_as(x, y)) // If the score is 0 it means we are at the finish
  {
    // We need a default fallback option if nothing is good enough
    int lowest_as = as = get_as(x+1, y);
    int dx = 1, dy = 0; // The difference in x/y we want from calculating the best antiscore

    if ((as = get_as(x-1, y)) <= lowest_as)
    {
      dx = -1;
      dy = 0;
      lowest_as = as;
    }
    if ((as = get_as(x, y+1)) <= lowest_as)
    {
      dx = 0;
      dy = 1;
      lowest_as = as;
    }
    if ((as = get_as(x, y-1)) <= lowest_as)
    {
      dx = 0;
      dy = -1;
      lowest_as = as;
    }

    x += dx;
    y += dy;
    
    map.cells[y*map.w + x] = 'x';
    
    print_map();
    
    puts("\n");

    sleep(1);
  }


  return 0;
}
