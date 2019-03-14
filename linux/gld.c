// gld -- glx opengl graphics server

// XXX initial implementation

#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <netinet/in.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

typedef unsigned int uint;

enum {
  HANGUP, WINSHUT, REDRAW, SHAPE, KEY, LDOWN, LUP, LMOUSE, RDOWN, RUP, RMOUSE,
  KEY_HOME = 0x101, KEY_IC, KEY_DC, KEY_END, KEY_PPAGE, KEY_NPAGE, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
  KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_MOUSE, KEY_F6, KEY_F7, KEY_F8, KEY_SHIFT = 0x200, KEY_CTRL = 0x400,
    
  M_winset = 20, M_prefposition, M_prefsize,    M_winopen,     M_wintitle,    M_winclose,     M_swapbuffers, 
  M_TexEnvi,     M_Viewport,     M_MatrixMode,  M_LoadMatrixf, M_MultMatrixf, M_LoadIdentity,
  M_Frustum,     M_Clear,        M_Translatef,  M_Rotatef,     M_Begin,       M_End,
  M_Vertex,      M_Color,        M_Normal,      M_TexCoord,    M_Ortho,       M_TexEnvfv,
  M_CallLists,   M_ListBase,     M_NewList,     M_EndList,     M_PushMatrix,  M_PointSize,
  M_PopMatrix,   M_Scalef,       M_Flush,       M_ClearColor,  M_ClearDepth,  M_DepthFunc,    
  M_Enable,      M_Disable,      M_ShadeModel,  M_Hint,        M_Materialfv,  M_Materialf,    
  M_Lightfv,     M_CullFace,     M_BindTexture, M_GenTextures, M_BlendFunc,   M_TexParameteri, 
  M_PixelStorei, M_TexImage2D,   M_AlphaFunc,  
};

typedef struct _Win {
  int id;
  Window win;
  GLXContext ctx;
  int w, h;
  struct _Win *next;
} Win;

GLXContext rootctx;
Display *display;
Window root;
XVisualInfo *visinfo;

int quit;
int nrx;
int debug;
int verbose;
int rxlen;

int pos_x;
int pos_y;
int pos_w = 300;
int pos_h = 300;

int sd;
Win *curwin;
Win *winlist;

void fatal(char *s)
{
  printf("Fatal error: %s\n",s);
  exit(9);
}

void winset(int id)
{
  Win *w;
  for (w = winlist; w; w = w->next) if (w->id == id) goto found;
  return;
found:
  glFlush();
  glXMakeCurrent(display, w->win, w->ctx);
  curwin = w;
}

void winclose(int id)
{
  Win *w, **wp;
  if (debug) printf("winclose(%d)\n",id);
  for (wp = &winlist; *wp; wp = &((*wp)->next)) if ((*wp)->id == id) goto found;
  return;
found:
  w = *wp; *wp = w->next;
  glFlush();

  if (curwin == w) { curwin = 0; glXMakeCurrent(display, None, NULL); }
  else if (curwin) glXMakeCurrent(display, curwin->win, curwin->ctx);
  if (w->ctx != rootctx) glXDestroyContext(display, w->ctx);
  XDestroyWindow(display, w->win);
  free(w);
}

void winopen(int id, char *title) // XXX needs work
{
  Win *w;
  XSetWindowAttributes attr;
  unsigned long mask;
  XSizeHints sizehints;
  Atom wmDelete;

  winclose(id);
  if (!(w = (Win *) malloc(sizeof(Win)))) fatal("malloc(Win)");
  memset(w, 0, sizeof(Win));

  w->id = id;
  w->next = winlist; 
  winlist = curwin = w;

  memset(&attr, 0, sizeof(attr));
  attr.colormap = XCreateColormap(display, root, visinfo->visual, AllocNone);
  attr.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | 
    ExposureMask | Button1MotionMask | Button3MotionMask | KeyPressMask;
//  mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
//  mask = CWBorderPixel | CWBitGravity | CWEventMask;
  mask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;
  w->win = XCreateWindow(display, root, pos_x, pos_y, pos_w, pos_h, 0, visinfo->depth, InputOutput, visinfo->visual, mask, &attr);

  wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);                 
  XSetWMProtocols(display, w->win, &wmDelete, 1);                            

  sizehints.x = pos_x;
  sizehints.y = pos_y;
  sizehints.width  = pos_w;
  sizehints.height = pos_h;
  sizehints.flags = USSize | USPosition;
  XSetNormalHints(display, w->win, &sizehints);
  XSetStandardProperties(display, w->win, title, title, None, NULL, 0, &sizehints);

  if (!(w->ctx = glXCreateContext(display, visinfo, rootctx, True))) fatal("glXCreateContext()");
  if (!rootctx) rootctx = w->ctx;

  XMapWindow(display, w->win);
  glXMakeCurrent(display, w->win, w->ctx);  
}

void txmsg(int msg, int win, int p1, int p2)
{
  int32_t m[4] = {msg, win, p1, p2}; // XXX shrink this?
  if (send(sd, (char *) m, 16, 0) != 16) { if (debug) printf("txmsg() failed quit=1\n"); quit = 1; }
}

int rxbuf(int sd, char *p, int n) // XXX get rid of?
{
  static char buf[16384]; int r;
  while (rxlen < n && rxlen < sizeof(buf)) {
    if ((r = recv(sd, buf + rxlen, sizeof(buf) - rxlen, 0)) <= 0) return r;
    rxlen += r;
  }
  if (n > rxlen) n = rxlen;
  memcpy(p, buf, n);
  if (rxlen > n) memcpy(buf, buf + n, rxlen - n);
  rxlen -= n;
  return n;
}

void rx(void *p, int n)
{
  int r;
  while (n) {
    if ((r = rxbuf(sd, (char *) p, n)) <= 0) { if (debug) printf("rx() failed quit=1\n"); quit = 1; return; }
    nrx += r;
    n -= r;
    p += r; 
  }
}
int32_t rx4(void)
{
  int32_t i;
  if (rxbuf(sd, (char *) &i, 4) != 4) { if (debug) printf("rx4() failed quit=1\n"); quit = 1; return -1; }
  nrx += 4;
  return i;
}

void rxmsg(void)
{
  union { int32_t i; float f; } p[16]; int n;
  static char s[2048]; // XXX
  char *pixels;
  
  switch (rx4()) {
  case M_prefsize:      rx(p, 8); pos_w = p[0].i; pos_h = p[1].i; break;
  case M_prefposition:  rx(p, 8); pos_x = p[0].i; pos_y = p[1].i; break;
  case M_winopen:       rx(p, 8); rx(s, p[1].i); winopen(p[0].i, s); break;
  case M_wintitle:      rx(s, rx4()); if (curwin) XStoreName(display, curwin->win, s); break;
  case M_winset:        winset(rx4()); break;
  case M_winclose:      winclose(rx4()); break;
  case M_swapbuffers:   if (curwin) glXSwapBuffers(display, curwin->win); // XXX need any glFlush(); glFinish(); glXWaitGL(); ?
                        if (debug) printf("swapbuffers() id=%d nrx=%d\n", curwin ? curwin->id : -1, nrx); nrx = 0; break;
  case M_Flush:         glFlush(); break;
  case M_Viewport:      rx(p, 16); glViewport(p[0].i, p[1].i, p[2].i, p[3].i); break;
  case M_Begin:         glBegin(rx4()); break;
  case M_End:           glEnd(); break;  
  case M_Vertex:        rx(p, 12); glVertex3fv((float *) p); break;
  case M_Color:         rx(p, 16); glColor4fv((float *) p); break;
  case M_Normal:        rx(p, 12); glNormal3fv((float *) p); break;
  case M_TexCoord:      rx(p, 8);  glTexCoord2fv((float *) p); break;
  case M_LoadIdentity:  glLoadIdentity(); break;
  case M_PushMatrix:    glPushMatrix(); break;
  case M_PopMatrix:     glPopMatrix(); break;  
  case M_MatrixMode:    glMatrixMode(rx4()); break;
  case M_LoadMatrixf:   rx(p, 64); glLoadMatrixf((float *) p); break;
  case M_MultMatrixf:   rx(p, 64); glMultMatrixf((float *) p); break;
  case M_Rotatef:       rx(p, 16); glRotatef(p[0].f, p[1].f, p[2].f, p[3].f); break;
  case M_Translatef:    rx(p, 12); glTranslatef(p[0].f, p[1].f, p[2].f); break;
  case M_Scalef:        rx(p, 12); glScalef(p[0].f, p[1].f, p[2].f); break;
  case M_Frustum:       rx(p, 24); glFrustum(p[0].f, p[1].f, p[2].f, p[3].f, p[4].f, p[5].f); break;
  case M_Ortho:         rx(p, 24); glOrtho(p[0].f, p[1].f, p[2].f, p[3].f, p[4].f, p[5].f); break;
  case M_ShadeModel:    glShadeModel(rx4()); break;
  case M_Lightfv:       rx(p, 24); glLightfv(p[0].i, p[1].i, &(p[2].f)); break;
  case M_Enable:        glEnable(rx4()); break;
  case M_Disable:       glDisable(rx4()); break;
  case M_CullFace:      glCullFace(rx4()); break;
  case M_Clear:         glClear(rx4()); break;  
  case M_ClearColor:    rx(p, 16); glClearColor(p[0].f, p[1].f, p[2].f, p[3].f); break;
  case M_ClearDepth:    rx(p, 4); glClearDepth(p[0].f); break;
  case M_DepthFunc:     glDepthFunc(rx4()); break;
  case M_Hint:          rx(p, 8); glHint(p[0].i, p[1].i); break;
  case M_CallLists:     rx(s, ((n=rx4())+3)&-4); glCallLists(n, GL_UNSIGNED_BYTE, s); break;
  case M_ListBase:      glListBase(rx4()); break;
  case M_NewList:       rx(p, 8); glNewList(p[0].i, p[1].i); break;
  case M_EndList:       glEndList(); break;
  case M_BindTexture:   rx(p, 8); glBindTexture(p[0].i, p[1].i); break;
  case M_BlendFunc:     rx(p, 8); glBlendFunc(p[0].i, p[1].i); break;
  case M_TexParameteri: rx(p, 12); glTexParameteri(p[0].i, p[1].i, p[2].i); break;
  case M_PixelStorei:   rx(p, 8); glPixelStorei(p[0].i, p[1].i); break;
  case M_TexImage2D:    rx(p, 32);
    n = p[3].i * p[4].i * 4;
    if (!(pixels = malloc(n))) fatal("malloc(pixels)");
    rx(pixels, n);
    glTexImage2D(p[0].i, p[1].i, p[2].i, p[3].i, p[4].i, p[5].i, p[6].i, p[7].i, pixels);
    free(pixels);
    break;

  case M_TexEnvi:       rx(p, 12); glTexEnvi(p[0].i, p[1].i, p[2].i); break;
  case M_TexEnvfv:      rx(p, 24); glTexEnvfv(p[0].i, p[1].i, &(p[2].f)); break;
  case M_Materialf:     rx(p, 12); glMaterialf(p[0].i, p[1].i, p[2].f); break;
  case M_Materialfv:    rx(p, 24); glMaterialfv(p[0].i, p[1].i, &(p[2].f)); break;
  case M_AlphaFunc:     rx(p, 8); glAlphaFunc(p[0].i, p[1].f); break;
  case M_PointSize:     rx(p, 4); glPointSize(p[0].f); break;
  case -1: break;
  default: fatal("rxmsg() unknown message");
  }
}

Win *findwindow(Window win)
{
  Win *w;
  for (w = winlist; w; w = w->next) if (w->win == win) return w;
  return 0;
}
static void event_loop()
{
  XEvent event; char buffer[10]; int n, k;
  Win *w;
  struct pollfd pfd[2];
  KeySym keysym;
  int i = 0;

  memset(pfd, 0, sizeof(pfd));
  pfd[0].fd = sd;
  pfd[0].events = POLLIN;
  pfd[1].fd = ConnectionNumber(display);
  pfd[1].events = POLLIN;
  
  while (!quit) {
    while (XPending(display) > 0) {
      XNextEvent(display, &event);
      switch (event.type) {
      case Expose:
        if (debug) printf("XNextEvent() Expose %d\n",i++);
        if (!event.xexpose.count && (w = findwindow(event.xexpose.window)))
          txmsg(REDRAW, w->id, w->w, w->h);
        break;
      case ConfigureNotify:
        if (debug) printf("XNextEvent() ConfigureNotify %d\n",i++);
        if ((w = findwindow(event.xconfigure.window)) && (w->w != event.xconfigure.width || w->h != event.xconfigure.height))
          txmsg(SHAPE, w->id, w->w = event.xconfigure.width, w->h = event.xconfigure.height);
        break;
      case ButtonPress:
        if (debug) printf("XNextEvent() ButtonPress %d\n",i++);
        if (w = findwindow(event.xbutton.window))
          txmsg((event.xbutton.button == Button1) ? LDOWN : RDOWN, w->id, event.xbutton.x, w->h - event.xbutton.y);
        break;
      case ButtonRelease:
        if (debug) printf("XNextEvent() ButtonRelease %d\n",i++);
        if (w = findwindow(event.xbutton.window))
          txmsg((event.xbutton.button == Button1) ? LUP : RUP, w->id, event.xbutton.x, w->h - event.xbutton.y);
        break;
      case MotionNotify:
        if (debug) printf("XNextEvent() MotionNotify %d\n",i++);
        if (w = findwindow(event.xmotion.window))
          txmsg((event.xmotion.state & Button1Mask) ? LMOUSE : RMOUSE, w->id, event.xmotion.x, w->h - event.xmotion.y);
        break;
      case KeyPress:
        if (debug) printf("XNextEvent() KeyPress %d\n",i++);
        if (w = findwindow(event.xkey.window)) {
          if (XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, 0) > 0)
            k = buffer[0];
          else {
            switch (keysym) {
            case XK_F1:     k = KEY_F1; break;
            case XK_F2:     k = KEY_F2; break;
            case XK_F3:     k = KEY_F3; break;
            case XK_F4:     k = KEY_F4; break;
            case XK_F5:     k = KEY_F5; break;
            case XK_F6:     k = KEY_F6; break;
            case XK_F7:     k = KEY_F7; break;
            case XK_F8:     k = KEY_F8; break;
            case XK_Up:     k = KEY_UP; break;
            case XK_Down:   k = KEY_DOWN; break;
            case XK_Right:  k = KEY_RIGHT; break;
            case XK_Left:   k = KEY_LEFT; break;
            case XK_Prior:  k = KEY_PPAGE; break;
            case XK_Next:   k = KEY_NPAGE; break;
            case XK_Home:   k = KEY_HOME; break;
            case XK_End:    k = KEY_END; break;
            case XK_Insert: k = KEY_IC; break;
            case XK_Delete: k = KEY_DC; break;
            default:        continue;
            }
            if (event.xkey.state & ShiftMask)   k |= KEY_SHIFT;
            if (event.xkey.state & ControlMask) k |= KEY_CTRL;
          }
          txmsg(KEY, w ? w->id : 0, k, 0);
        }
        break;
      case ClientMessage:
        if (debug) printf("XNextEvent() ClientMessage %d\n",i++);
        if (!strcmp(XGetAtomName(display, event.xclient.message_type), "WM_PROTOCOLS") && (w = findwindow(event.xclient.window)))
          txmsg(WINSHUT, w->id, 0, 0);
        break;
      }
      if (quit) return;
    }

    if ((n = poll(pfd, 2, -1)) < 0) { if (debug) printf("poll()=%d!\n",n); return; } // XXX reorganize this logic
    if (pfd[0].revents)
    {
      rxmsg();
      while (rxlen) {
        rxmsg();
        if (quit) return;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  int i, ld; Win *w, *n;
  char *dpyname = 0;
  struct sockaddr_in addr;
  int port = 5001;
  int attribs[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 16, None };
  int scrnum;

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-v")) verbose = 1;
    else if (!strcmp(argv[i],"-d")) verbose = debug = 1;
    else if (!strcmp(argv[i],"-p") && i+1 < argc) port = atoi(argv[++i]);
    else if (!strcmp(argv[i],"-display") && i+1 < argc) dpyname = argv[++i];
    else fatal("usage: gld [-v] [-d] [-p port] [-display display]");
  }
  
  if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) fatal("socket()");
  i = 1; setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // XXX
  if (bind(ld, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) fatal("bind()");
  if (listen(ld, 1) < 0) fatal("listen()");

  if (verbose) printf("Listening on port %d...\n", port);
  for (;;) {
    if ((sd = accept(ld, 0, 0)) < 0) fatal("accept()");
    if (verbose) printf("Accepted client connection.\n") ;
    i = 1; setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));

    if ((i = fork()) < 0) fatal("fork()");
    if (!i) {
      close(ld);
      if (!(display = XOpenDisplay(dpyname))) fatal("XOpenDisplay()");
      scrnum = DefaultScreen(display);
      root = RootWindow(display, scrnum);

      if (!glXQueryExtension(display, 0, 0)) fatal("X Server doesn't support GLX extension");
      if (!(visinfo = glXChooseVisual(display, scrnum, attribs))) fatal("couldn't get an RGB, Double-buffered visual\n");
      
      event_loop();

      for ((w = winlist) && (n = w->next); w; (w = n) && (n = w->next)) winclose(w->id);
    
      if (sd >= 0) close(sd);
      if (verbose) printf("Client closed.\n");
      XCloseDisplay(display); 
      exit(0); 
    }
    close(sd);
  }
}
