#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <X11/Xlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <time.h>

// How many neurons can a neuron see in one direction
#define NEURON_SIGHT 8
// How many bytes we shift the factor sum
#define FACTOR_SHIFT 6

// Decay speed affects how fast neuron firing decays in the brain, if the speed is too slow neurons may get into an equalibrium of firing, and will not escape it, so the faster the speed the more active the brain becomes.
#define DECAY_SPEED 32

#define WND_W 256
#define WND_H 128

// 0=blue, 1=green, 2=red
#define COLOR 2

// Writable frame buffer, essentially blitting:
// https://bbs.archlinux.org/viewtopic.php?id=225741

Display* xdsp = 0;
int xscr;
Visual* xvisual;
Atom wmdeletewindow_atom;
GC xgc;
Window xwnd;
XImage* ximg;

unsigned char* ximg_data;

/**
 * NOTE: After we do all the factoring, we shift the value a constant number of bytes to the right, which is equivalent to dividing, which is equivalent to making the `factors` array a fixed fraction number.
*/
typedef struct
{
  char factors[(NEURON_SIGHT*2+1)*(NEURON_SIGHT*2+1)]; // Making a rectangle around the neuron
  u_char bias;
} node_t;

node_t* nodes;

void free_window()
{
	XDestroyWindow(xdsp, xwnd);
	
	if (ximg)
		XDestroyImage(ximg);
	ximg = 0;

	XCloseDisplay(xdsp);
  free(nodes);
}

int x_error_handler(Display * d, XErrorEvent * e)
{
  static char text[512];
  printf("x_error_handler(): %hhu\n", e->error_code);
  XGetErrorText(d, e->error_code, text, 512);
  puts(text);
  return 0;
}

int init_window()
{
  XSetErrorHandler(x_error_handler);

	xdsp = XOpenDisplay(NULL);
	if (!xdsp)
  {
		return 0;
  }

	xscr = DefaultScreen(xdsp);

	// Custom WM_DELETE_WINDOW protocol
	wmdeletewindow_atom = XInternAtom(xdsp, "WM_DELETE_WINDOW", False);

	// Visuals and stuff for drawing.
	XVisualInfo xvisualinfo;
	if (!XMatchVisualInfo(xdsp, xscr, 24, TrueColor, &xvisualinfo))
  {
		free_window();
		puts("init_window(): Failed to XMatchVisualInfo(). Are you sure you have 24 bit depth TrueColor?");
		return 0;
	}

	xvisual = xvisualinfo.visual;

  XSetWindowAttributes attribs = {0};
	attribs.event_mask = KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;
	
	xwnd = XCreateWindow(
		xdsp,
		RootWindow(xdsp, xscr),
		0, 0, WND_W, WND_H,
		0, // border width
		24, // bit depth
		InputOutput,
		xvisual,
		CWBorderPixel | CWEventMask,
		&attribs
	);
	
	// XDefineCursor(xdsp, xwnd, createnullcursor(xdsp, xwnd));
	
	// GC Creation, before mapping.
	XGCValues xgcvalues;
	xgcvalues.graphics_exposures = False;
	xgc = XCreateGC(xdsp, xwnd, GCGraphicsExposures, &xgcvalues );

	XSelectInput(xdsp, xwnd, attribs.event_mask);

	xwnd = xwnd;

	// Set our custom WM_DELETE_WINDOW protocol
	XSetWMProtocols(xdsp, xwnd, &wmdeletewindow_atom, 1);

	XMapWindow(xdsp, xwnd);
	XFlush(xdsp); // Wait until window is visible.

	// Image creation
	char* data = malloc(WND_W*32/8 * WND_H); // 32 because of padding >:(
	if (!data)
  {
		free_window();
		puts("init_window(): Failed to allocate image data.");
		return 0;
	}

	ximg = XCreateImage(
		xdsp,
		xvisual,
		24,
		ZPixmap,
		0,
		data,
		WND_W, WND_H,
		32, // bitmap_pad, no clue, 32 is "if unsure".
		WND_W*32/8
	); // bytes per scanline, since we have padding of 32, we use 32 instead of 24.

	if (!ximg)
  {
		free_window();
		puts("init_window(): Failed to create image.");
		return 0;
	}

  ximg_data = ximg->data;

  // Limit window size
  XSizeHints size_hints;
  size_hints.flags = PMinSize | PMaxSize;
  size_hints.min_width = WND_W;
  size_hints.max_width = WND_W;
  size_hints.min_height = WND_H;
  size_hints.max_height = WND_H;
  XSetWMNormalHints(xdsp, xwnd, &size_hints);

	return 1;	
}

void get_screen_size(int* w, int* h)
{
	XWindowAttributes ra;
	XGetWindowAttributes(xdsp, DefaultRootWindow(xdsp), &ra);
	*w = ra.width;
	*h = ra.height;
}

// x y to index
int xyi(int x, int y)
{
  if (x < 0)
  {
    x += WND_W;
  }
  else if (x >= WND_W)
  {
    x -= WND_W;
  }
  
  if (y < 0)
  {
    y += WND_H;
  }
  else if (y >= WND_H)
  {
    y -= WND_H;
  }

  return y*WND_W + x;
}

void set_pixel(int i, char r, char g, char b)
{
	ximg_data[i*4+0] = b;
	ximg_data[i*4+1] = g;
	ximg_data[i*4+2] = r;
}

static inline void decay_channel(unsigned char* c)
{
  if (*c >= DECAY_SPEED)
  {
  	*c -= DECAY_SPEED;
  }
  else
  {
    *c = 0;
  }
}

void decay_img()
{
  for (int i = 0; i < WND_W*WND_H*4; i+=4)
  {
    if (ximg_data[i+0] == 0)
    {
      if (ximg_data[i+1] == 0)
      {
        decay_channel(&ximg_data[i+2]);
      }
      else
      {
        decay_channel(&ximg_data[i+1]);
      }
    }
    else
    {
      decay_channel(&ximg_data[i+0]);
    }
  }
}

// Returns how fired the neuron is from 0 to 255
static inline int get_fire(int i)
{
  return ((int)ximg_data[i*4+2]+ximg_data[i*4+1]+ximg_data[i*4+0])/3;
}

// Returns if it fired, meaning it's above a specific threshold to be considered fired
static inline int get_if_fire(int i)
{
  return get_fire(i) >= 230;
}

// FULL POWER
static inline void fire(int i)
{
  ximg_data[i*4+2] = 255;
  ximg_data[i*4+1] = 255;
  ximg_data[i*4+0] = 255;
}

// NO POWER
static inline void unfire(int i)
{
  ximg_data[i*4+2] = 0;
  ximg_data[i*4+1] = 0;
  ximg_data[i*4+0] = 0;
}

// In monochrome
void put_img()
{
	XPutImage
  (
		xdsp,
		xwnd,
		xgc,
		ximg,
		0, 0,
		0, 0,
		WND_W, WND_H
	);
}

void run_window()
{
	XEvent e;
  static int mouse = 0, mousex = 0, mousey = 0;

	while (XPending(xdsp))
  {
		XNextEvent(xdsp, &e);

		switch (e.type)
    {
			case ClientMessage:
			if (!(
				e.xclient.message_type == XInternAtom(xdsp, "WM_PROTOCOLS", True) && 
				(Atom)e.xclient.data.l[0] == wmdeletewindow_atom
			))
				break;
			// Otherwise proceed here(most likely it wont, we overrode it, but just incase):
			case DestroyNotify:
			exit(0);
			break;
			
			case KeyPress:
      for (int i = 0; i < WND_W*WND_H/4; i++)
        set_pixel(rand()%(WND_H*WND_W), rand()%2?255:0, 0, 0);
      break;
			
      case KeyRelease:
      break;

      case ButtonPress:
      if (e.xbutton.button==Button1)
      {
        mouse = 1;
      }
      else if (e.xbutton.button==Button3)
      {
        mouse = 2;
      }
      break;

      case ButtonRelease:
      mouse = 0;
      break;

      case MotionNotify:
      mousex = e.xbutton.x;
      mousey = e.xbutton.y;
			break;
		}
	}

  if (mouse)
  {
    int i = xyi(mousex, mousey);
    if (mouse == 1)
    {
      fire(i);
    }
    else
    {
      unfire(i);
    }
  }
}

static inline int min(int x, int y)
{
  return x<y?x:y;
}
static inline int max(int x, int y)
{
  return x>y?x:y;
}

void init_nodes()
{
  nodes = malloc(WND_W*WND_H*sizeof(node_t));


  for (int i = 0; i < WND_W*WND_H; i++)
  {
    nodes[i].bias = rand();
    for (int j = 0; j < sizeof(nodes[0].factors)/sizeof(nodes[0].factors[0]); j++)
    {
      nodes[i].factors[j] = rand();
    }
  }
  for (int i = 0; i < WND_W*WND_H; i++)
  {
    if (rand()&1)
      fire(i);
    else
      unfire(i);
  }
}

void run_nodes_to_img(int seed)
{
  int i = 0;
  for (int y = 0; y < WND_H; y++)
  {
    for (int x = 0; x < WND_W; x++, i++)
    {
      // Run the node
      long sum = 0;

      // We need a j and a k to access both the real index in the ximg and the relative index in factors!
      int k = 0;
      for (int v = y-NEURON_SIGHT; v <= y+NEURON_SIGHT; v++)
      {
        for (int u = x-NEURON_SIGHT; u <= x+NEURON_SIGHT; u++, k++)
        {
          int j = xyi(u, v);

          if (j != i && get_if_fire(j)) // Make sure to igore THIS neuron
          {
            sum+=nodes[i].factors[k];
          }
        }
      }
      
      if (sum & (1<<(sizeof(int)*8-1))) // If sum is negative
      {
        sum = 0;
      }
      else
      {
        sum >>= FACTOR_SHIFT;
      }

      int did_fire;
      did_fire = sum > (long)nodes[i].bias;

      // Fire together? Wire together. Out of sync? Synapses shrink!
      k = 0;
      for (int v = y-NEURON_SIGHT; v <= y+NEURON_SIGHT; v++)
      {
        for (int u = x-NEURON_SIGHT; u <= x+NEURON_SIGHT; u++, k++)
        {
          int j = xyi(u, v);

          if (j != i) // Make sure to igore THIS neuron
          {
            if (did_fire & (get_fire(j) > 220)) // If both fired strengthen
            {
              nodes[i].factors[k] += nodes[i].factors[k]==255?0:1;
            }
            else if (!did_fire & (get_fire(j) < 220))
            {
              nodes[i].factors[k] -= nodes[i].factors[k]==0?0:1;
            }
          }
        }
      }
      
      // Going haywire? It will tire. Too quiet? 
      int last_fire = get_fire(i);
      if (last_fire <= 0 && !did_fire)
      {
        nodes[i].bias -= nodes[i].bias==0?0:1;
      }
      else if (last_fire >= 128 && did_fire)
      {
        nodes[i].bias += nodes[i].bias==255?0:1;
      }

      // Put it on the image
      if (did_fire)
      {
        fire(i);
      }
      if (i == 1)
      {
        printf("%d\n", nodes[i].bias);
      }
    }
  }
}

int main()
{
  init_window();
  init_nodes();
  
  int frames = 0;
  time_t last_time = time(NULL), t;
  char title[64];
  while (1)
  {
    decay_img();
    run_nodes_to_img(frames);
    run_window();
    put_img();
    
    frames++;
    if ((t = time(NULL)) > last_time)
    {
      last_time = t;
      sprintf(title, "Brain : %dfps", frames);
      frames = 0;
      XStoreName(xdsp, xwnd, title);
    }
  }
}
