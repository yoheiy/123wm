/* up to three window manager */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
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
Window   main_w, left_w = None, right_w = None;
struct my_hint main_h;
const char * const bind_keys = "JKLHMRTU";

int screen_width, screen_height;

struct my_hint
get_size_hints(Window w)
{
   XSizeHints     hints;
   long           supplied;
   struct my_hint h;

   XGetWMNormalHints(dpy, w, &hints, &supplied);
   h.w_base = (supplied & PBaseSize)  ? hints.base_width  :
              (supplied & PMinSize)   ? hints.min_width   : 32;
   h.h_base = (supplied & PBaseSize)  ? hints.base_height :
              (supplied & PMinSize)   ? hints.min_height  : 32;
   h.w_inc  = (supplied & PResizeInc) ? hints.width_inc   : 1;
   h.h_inc  = (supplied & PResizeInc) ? hints.height_inc  : 1;
   h.w_min  = (supplied & PMinSize)   ? hints.min_width   : 32;
   h.h_min  = (supplied & PMinSize)   ? hints.min_height  : 32;
   if (h.w_min == 0) h.w_min = 32;
   if (h.h_min == 0) h.h_min = 32;
   if (h.w_inc == 0) h.w_inc = 1;
   if (h.h_inc == 0) h.h_inc = 1;
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
max_h(struct my_hint h)
{
   const int t0 = screen_height;
   const int t1 = t0 - h.h_base;
   const int t2 = t1 / h.h_inc;
   const int t3 = t2 * h.h_inc + h.h_base;

   return t3;
}

void
main_normalize(Window w, struct my_hint h)
{
   struct xywh a;

   a.x = 0;
   a.w = max_w(h);
   a.h = h.h_min;
   a.y = screen_height - h.h_min;
   apply_geom(w, a);
}

void
main_maximize(Window w, struct my_hint h)
{
   struct xywh a;

   a.x = 0;
   a.w = max_w(h);
   a.h = max_h(h);
   a.y = screen_height - a.h;
   apply_geom(w, a);
   XRaiseWindow(dpy, w);
}

void
sub_place_sub(Window w0, struct my_hint h, int x, unsigned int w)
{
   struct xywh a;

   a.x = x;
   a.w = h.w_base + (w - h.w_base) / h.w_inc * h.w_inc;
   a.y = 0;
   a.h = h.h_base + (screen_height - main_h.h_min - h.h_base) / h.h_inc
                                                              * h.h_inc;
   apply_geom(w0, a);
}

void
sub_place(Window w0, struct my_hint h, Bool left)
{
   int x = left ? 0 : screen_width / 2;
   unsigned int w = screen_width / 2;

   sub_place_sub(w0, h, x, w);
}

void place_world_one(void);

void
place_world(void)
{
   struct my_hint h;

   if (left_w  == None) return;
   if (right_w == None) return place_world_one();

   h = get_size_hints(left_w);
   sub_place(left_w, h, True);
   XRaiseWindow(dpy, left_w);

   h = get_size_hints(right_w);
   sub_place(right_w, h, False);
   XRaiseWindow(dpy, right_w);
}

void
sub_place_one(Window w, struct my_hint h)
{
   sub_place_sub(w, h, 0, screen_width);
   XRaiseWindow(dpy, w);
}

void
place_world_one(void)
{
   struct my_hint h;

   h = get_size_hints(left_w);
   sub_place_one(left_w, h);
}

void
set_focus(Window w)
{
   if (w == None) return;
   XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
}

void
swap_pane(void)
{
   Window t;

   t = left_w;
   left_w = right_w;
   right_w = t;
}

int
is_a_viewable_window(Window w)
{
   XWindowAttributes wa;

   if (w == main_w)  return 0;
   if (w == left_w)  return 0;
   if (w == right_w) return 0;
   XGetWindowAttributes(dpy, w, &wa);
   if (wa.override_redirect == True) return 0;
   if (wa.map_state != IsViewable)   return 0;
}

Window
find_window(Bool first)
{
   Window r_root, r_parent, *r_ch, ret = None;
   unsigned int n_ch, i;

   if (!XQueryTree(dpy, root, &r_root, &r_parent, &r_ch, &n_ch))
      return None;

   for (i = 0; i < n_ch; i++)
      if (is_a_viewable_window(r_ch[i])) {
         ret = r_ch[i];
         if (first) break; }

   XFree(r_ch);
   return ret;
}

Window
find_first_window(void)
{
   return find_window(/* first = */ True);
}

Window
find_last_window(void)
{
   return find_window(/* first = */ False);
}

void
switch_window(Bool next)
{
   Window w;

   w = next ? find_first_window() : find_last_window();
   if (w == None) return;
   if (!next)
      XLowerWindow(dpy, left_w);
   left_w = w;
   set_focus(left_w);
   if (next)
      XRaiseWindow(dpy, left_w);
}

void
next_window(void)
{
   switch_window(/* next = */ True);
}

void
prev_window(void)
{
   switch_window(/* next = */ False);
}

void
key_event_handler(KeySym k)
{
   switch (k) {
   case XK_m:
      place_world();
      main_maximize(main_w, main_h);
      set_focus(main_w);
      break;
   case XK_semicolon:
      main_normalize(main_w, main_h);
      set_focus(main_w);
      break;
   case XK_j:
      main_normalize(main_w, main_h);
      next_window();
      place_world();
      break;
   case XK_k:
      main_normalize(main_w, main_h);
      prev_window();
      place_world();
      break;
   case XK_l:
      main_normalize(main_w, main_h);
      place_world();
      set_focus(right_w);
      break;
   case XK_h:
      main_normalize(main_w, main_h);
      place_world();
      set_focus(left_w);
      break;
   case XK_r:
      main_normalize(main_w, main_h);
      place_world();
      set_focus(PointerRoot);
      break;
   case XK_t:
      main_normalize(main_w, main_h);
      swap_pane();
      if (left_w == None) next_window();
      place_world();
      break;
   case XK_u:
      main_normalize(main_w, main_h);
      right_w = None;
      place_world();
      set_focus(left_w);
   }
}

KeySym
key_convert(XEvent e)
{
   KeySym k;
   int a;

   a = e.xkey.state & ShiftMask ? 1 : 0;
   k = XLookupKeysym(&e.xkey, a);

   return k;
}

void
destroy_event_handler(Window w)
{
   if (w == left_w)  left_w  = None;
   if (w == right_w) right_w = None;
}

void
mainloop_body(void)
{
   XEvent e;

   XNextEvent(dpy, &e);
   switch (e.type) {
   case KeyPress:
      key_event_handler(key_convert(e));
      break;
   case DestroyNotify:
      destroy_event_handler(e.xdestroywindow.window);
   }
   if (left_w == None)
      swap_pane();
}

void
mainloop(void)
{
   for (;;)
      mainloop_body();
}

void
grab_key(const char *s, int mask)
{
   XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym(s)), mask, root,
            True, GrabModeAsync, GrabModeAsync);
}

void
select_input(void)
{
   const char *c;
   char s[2] = "X";

   for (c = bind_keys; *c; c++) {
      s[0] = *c;
      grab_key(s, Mod4Mask);
      grab_key(s, Mod4Mask | ShiftMask); }

   grab_key("semicolon", Mod4Mask);

   XSelectInput(dpy, root, SubstructureNotifyMask);
}

void
new_window(Window w)
{
   XWindowAttributes wa;

   if (w == main_w) return;
   XGetWindowAttributes(dpy, w, &wa);
   if (wa.override_redirect == True) return;
   if (wa.map_state != IsViewable)   return;

   if (left_w == None)
      left_w = w;
   else if (right_w == None)
      right_w = w;
}

void
init_clients(void)
{
   Window r_root, r_parent, *r_ch;
   unsigned int n_ch, i;

   main_w = (Window)atoi(getenv("WINDOWID"));
   main_h = get_size_hints(main_w);

   if (!XQueryTree(dpy, root, &r_root, &r_parent, &r_ch, &n_ch))
      return;

   for (i = 0; i < n_ch; i++)
      new_window(r_ch[i]);

   XFree(r_ch);
}

void
init_wm(void)
{
   dpy = XOpenDisplay(NULL);
   if (!dpy) exit(1);

   root = DefaultRootWindow(dpy);
   screen_width  = XDisplayWidth(dpy,  DefaultScreen(dpy));
   screen_height = XDisplayHeight(dpy, DefaultScreen(dpy));
}

int
main(void)
{
   init_wm();
   init_clients();

   main_normalize(main_w, main_h);
   place_world();

   select_input();
   mainloop();

   XCloseDisplay(dpy);
   return 0;
}
