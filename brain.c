#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include <X11/Xlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

// How many neurons can a neuron see in one direction
#define NEURON_SIGHT 8
// How many bytes we shift the factor sum
#define FACTOR_SHIFT 4

// Decay speed affects how fast neuron firing decays in the brain, if the speed is too slow neurons may get into an equalibrium of firing, and will not escape it, so the faster the speed the more active the brain becomes.
#define DECAY_SPEED 32

// Writable frame buffer, essentially blitting:
// https://bbs.archlinux.org/viewtopic.php?id=225741

Display* xdsp = 0;
int xscr;
Visual* xvisual;
Atom wmdeletewindow_atom;
GC xgc;
Window xwnd;
XImage* ximg;
int wnd_w, wnd_h;

unsigned char* ximg_data;

/**
 * NOTE: After we do all the factoring, we shift the value a constant number of bytes to the right, which is equivalent to dividing, which is equivalent to making the `factors` array a fixed fraction number.
*/
typedef struct
{
  char factors[NEURON_SIGHT*NEURON_SIGHT*4]; // Making a rectangle around the neuron
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

int init_window_nodes(int w, int h)
{
  XSetErrorHandler(x_error_handler);

  wnd_w = w;
  wnd_h = h;
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
	attribs.event_mask = KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask;
	
	xwnd = XCreateWindow(
		xdsp,
		RootWindow(xdsp, xscr),
		0, 0, w, h,
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
	char* data = malloc(w*32/8 * h); // 32 because of padding >:(
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
		w, h,
		32, // bitmap_pad, no clue, 32 is "if unsure".
		w*32/8
	); // bytes per scanline, since we have padding of 32, we use 32 instead of 24.

	if (!ximg)
  {
		free_window();
		puts("init_window(): Failed to create image.");
		return 0;
	}

  ximg_data = ximg->data;

	XStoreName(xdsp, xwnd, "BRAIN");

  nodes = malloc(w*h*sizeof(node_t));

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
  return y*wnd_w + x;
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
  for (int i = 0; i < wnd_w*wnd_h*4; i+=4)
  {
    decay_channel(&ximg_data[i+0]);
    decay_channel(&ximg_data[i+1]);
    decay_channel(&ximg_data[i+2]);
  }
}

// Returns if a pixel is fired or not!
int get_fire(int i)
{
  return ximg_data[i*4+0] + ximg_data[i*4+1] + ximg_data[i*4+2];
}

// In monochrome
void update_img()
{
	XPutImage
  (
		xdsp,
		xwnd,
		xgc,
		ximg,
		0, 0,
		0, 0,
		wnd_w, wnd_h
	);
	
	// XSync(xdsp, False);
}

void run_window()
{
	XEvent e;

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
      for (int i = 0; i < 10000; i++)
        set_pixel(rand()%(wnd_h*wnd_w), rand()%2?255:0, 0, 0);
			case KeyRelease:
			break;
		}
	}
}

static inline unsigned min(unsigned x, unsigned y)
{
  return x<y?x:y;
}
static inline unsigned max(unsigned x, unsigned y)
{
  return x>y?x:y;
}

void run_nodes_to_img()
{
  int i = 0;
  for (int y = 0; y < wnd_h; y++)
  {
    // We need a j and a k to access both the real index in the ximg and the relative index in factors!
    int k = 0;
    for (int x = 0; x < wnd_w; x++, i++)
    {
      // Run the node
      long sum = 0;

      for (int v = max(y-NEURON_SIGHT, 0); v < min(y+NEURON_SIGHT+1, wnd_h); v++)
      {
        for (int u = max(x-NEURON_SIGHT, 0); u < min(x+NEURON_SIGHT+1, wnd_w); u++, k++)
        {
          int j = xyi(u, v);

          // printf("(%d,%d): %d\n",x,y, get_fire(j));
          if (j != i && get_fire(j)) // Make sure to igore THIS neuron
          {
            sum += nodes[i].factors[k];
          }
        }
      }
      if (sum & (1<<(sizeof(int)*8-1)))
      {
        continue;
      }
      sum >>= FACTOR_SHIFT;
        // printf("%d\n", sum);
      // if (sum)
      // Put it on the image
      if (sum > (unsigned)nodes[i].bias)
      {
        set_pixel(i, 255, 0, 0);
      }
    }
  }
}

int main()
{
  srand(time(NULL));

  init_window_nodes(260, 256);

  for (int i = 0; i < wnd_w*wnd_h; i++)
  {
    nodes[i].bias = rand();
    for (int j = 0; j < sizeof(nodes[0].factors)/sizeof(nodes[0].factors[0]); j++)
    {
      nodes[i].factors[j] = rand();
    }
  }
  for (int i = 0; i < wnd_w*wnd_h*4; i+=4)
  {
    ximg_data[i+2] = rand();
  }
  while(1)
  {
    decay_img();
    update_img();
    run_window();
    run_nodes_to_img();
  }
}