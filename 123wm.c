/* up to three window manager */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
struct my_hint {
   int w_base, w_inc, w_min;
   int h_base, h_inc, h_min;
};
struct xywh {
   int          x, y;
   unsigned int w, h;
};
Display *dpy;
Window   root;

int screen_width, screen_height;

struct my_hint
get_size_hints(Window w)
{
   XSizeHints     hints;
   long           supplied;
   struct my_hint h;

   XGetWMNormalHints(dpy, w, &hints, &supplied);
   h.w_base = hints.base_width;
   h.w_inc  = hints.width_inc;
   h.h_min  = hints.min_height;

   return h;
}

void
apply_geom(Window ww, struct xywh a)
{
   XMoveResizeWindow(dpy, ww, a.x, a.y, a.w, a.h);
}

int
max_w(struct my_hint h)
{
   const int t0 = screen_width;
   const int t1 = t0 - h.w_base;
   const int t2 = t1 / h.w_inc;
   const int t3 = t2 * h.w_inc + h.w_base;

   return t3;
}

int
main(void)
{
   Window         w;
   struct my_hint hints;
   struct xywh    a;

   dpy = XOpenDisplay(NULL);
   if (!dpy) return 1;

   screen_width  = XDisplayWidth(dpy,  DefaultScreen(dpy));
   screen_height = XDisplayHeight(dpy, DefaultScreen(dpy));

   w = (Window)atoi(getenv("WINDOWID"));
   hints = get_size_hints(w);
   a.x = 0;
   a.w = max_w(hints);
   a.h = hints.h_min;
   a.y = screen_height - hints.h_min;
   apply_geom(w, a);

   return 0;
}
