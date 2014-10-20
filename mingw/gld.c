// gld -- win32 opengl graphics server

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef OK_OPENGL
#include <GL/gl.h>
#endif

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
  HWND hWnd;
  HDC hDC;
  HGLRC hRC;
  HANDLE event;
  int w, h;
  struct _Win *next;
} Win;

HGLRC rootRC = NULL;
HINSTANCE hInstance;
HANDLE mutex;

int quit  = 0;
int nrx   = 0;
int debug = 0;
int verbose = 0;

int pos_x;
int pos_y;
int pos_w = 300;
int pos_h = 300;

int sd;
Win *curwin = 0;
Win *winlist = 0;

void fatal(char *s)
{
  printf("Fatal error: %s\n",s);
  exit(9);
}

#ifndef OK_OPENGL
#define GL_UNSIGNED_BYTE 0x1401   // XXX get rid of

typedef uint   GLenum;
typedef uint   GLbitfield;
typedef void   GLvoid;
typedef int    GLint;
typedef uint   GLuint;
typedef int    GLsizei;
typedef float  GLfloat;
typedef float  GLclampf;
typedef double GLdouble;
typedef double GLclampd;

void (APIENTRY *glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
void (APIENTRY *glMatrixMode)(GLenum mode);
void (APIENTRY *glLoadMatrixf)(const GLfloat *m);
void (APIENTRY *glMultMatrixf)(const GLfloat *m);
void (APIENTRY *glLoadIdentity)(void);
void (APIENTRY *glFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
void (APIENTRY *glClear)(GLbitfield mask);
void (APIENTRY *glTranslatef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *glRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *glBegin)(GLenum mode);
void (APIENTRY *glEnd)(void);
void (APIENTRY *glVertex3fv)(const GLfloat *v);
void (APIENTRY *glColor4fv)(const GLfloat *c);
void (APIENTRY *glNormal3fv)(const GLfloat *v);
void (APIENTRY *glTexCoord2fv)(const GLfloat *v);
void (APIENTRY *glOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void (APIENTRY *glCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
void (APIENTRY *glListBase)(GLuint base);
void (APIENTRY *glNewList)(GLuint list, GLenum mode);
void (APIENTRY *glEndList)(void);
void (APIENTRY *glPushMatrix)(void);
void (APIENTRY *glPopMatrix)(void);
void (APIENTRY *glScalef)(GLfloat x, GLfloat y, GLfloat z);
void (APIENTRY *glFlush)(void);
void (APIENTRY *glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (APIENTRY *glClearDepth)(GLclampd depth);
void (APIENTRY *glDepthFunc)(GLenum func);
void (APIENTRY *glEnable)(GLenum cap);
void (APIENTRY *glDisable)(GLenum cap);
void (APIENTRY *glShadeModel)(GLenum mode);
void (APIENTRY *glHint)(GLenum target, GLenum mode);
void (APIENTRY *glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
void (APIENTRY *glMaterialf)(GLenum face, GLenum pname, GLfloat param);
void (APIENTRY *glLightfv)(GLenum light, GLenum pname, const GLfloat *params);
void (APIENTRY *glCullFace)(GLenum face);
void (APIENTRY *glBindTexture)(GLenum target, GLuint texture);
void (APIENTRY *glGenTextures)(GLsizei n, GLuint *textures);
void (APIENTRY *glBlendFunc)(GLenum sfactor, GLenum dfactor);
void (APIENTRY *glTexParameteri)(GLenum target, GLenum pname, GLint param);
void (APIENTRY *glPixelStorei)(GLenum pname, GLint param);
void (APIENTRY *glTexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                              GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void (APIENTRY *glTexEnvi)(GLenum target, GLenum pname, GLint param);
void (APIENTRY *glTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
//void (APIENTRY *glActiveTexture)(uint texture);
void (APIENTRY *glAlphaFunc)(GLenum func, GLclampf ref);
void (APIENTRY *glPointSize)(GLfloat size);
//void (APIENTRY *glGetIntegerv)(GLenum pname, GLint *params);

HGLRC (WINAPI *xwglCreateContext)(HDC);
BOOL  (WINAPI *xwglDeleteContext)(HGLRC);
BOOL  (WINAPI *xwglMakeCurrent)(HDC, HGLRC);
BOOL  (WINAPI *xwglShareLists)(HGLRC, HGLRC);

void load_opengl(void)
{
  HMODULE dll;
  if (!(dll = LoadLibrary("opengl32.dll"))) fatal("LoadLibrary(opengl32.dll)");
  if (!(glViewport      = (void *) GetProcAddress(dll, "glViewport")))      fatal("GetProcAddress(glViewport)");
  if (!(glMatrixMode    = (void *) GetProcAddress(dll, "glMatrixMode")))    fatal("GetProcAddress(glMatrixMode)");
  if (!(glLoadMatrixf   = (void *) GetProcAddress(dll, "glLoadMatrixf")))   fatal("GetProcAddress(glLoadMatrixf)");
  if (!(glMultMatrixf   = (void *) GetProcAddress(dll, "glMultMatrixf")))   fatal("GetProcAddress(glMultMatrixf)");
  if (!(glLoadIdentity  = (void *) GetProcAddress(dll, "glLoadIdentity")))  fatal("GetProcAddress(glLoadIdentity)");
  if (!(glFrustum       = (void *) GetProcAddress(dll, "glFrustum")))       fatal("GetProcAddress(glFrustum)");
  if (!(glClear         = (void *) GetProcAddress(dll, "glClear")))         fatal("GetProcAddress(glClear)");
  if (!(glTranslatef    = (void *) GetProcAddress(dll, "glTranslatef")))    fatal("GetProcAddress(glTranslatef)");
  if (!(glRotatef       = (void *) GetProcAddress(dll, "glRotatef")))       fatal("GetProcAddress(glRotatef)");
  if (!(glBegin         = (void *) GetProcAddress(dll, "glBegin")))         fatal("GetProcAddress(glBegin)");
  if (!(glEnd           = (void *) GetProcAddress(dll, "glEnd")))           fatal("GetProcAddress(glEnd)");
  if (!(glVertex3fv     = (void *) GetProcAddress(dll, "glVertex3fv")))     fatal("GetProcAddress(glVertex3fv)");
  if (!(glColor4fv      = (void *) GetProcAddress(dll, "glColor4fv")))      fatal("GetProcAddress(glColor4fv)");
  if (!(glNormal3fv     = (void *) GetProcAddress(dll, "glNormal3fv")))     fatal("GetProcAddress(glNormal3fv)");
  if (!(glTexCoord2fv   = (void *) GetProcAddress(dll, "glTexCoord2fv")))   fatal("GetProcAddress(glTexCoord2fv)");
  if (!(glOrtho         = (void *) GetProcAddress(dll, "glOrtho")))         fatal("GetProcAddress(glOrtho)");
  if (!(glCallLists     = (void *) GetProcAddress(dll, "glCallLists")))     fatal("GetProcAddress(glCallLists)");
  if (!(glListBase      = (void *) GetProcAddress(dll, "glListBase")))      fatal("GetProcAddress(glListBase)");
  if (!(glNewList       = (void *) GetProcAddress(dll, "glNewList")))       fatal("GetProcAddress(glNewList)");
  if (!(glEndList       = (void *) GetProcAddress(dll, "glEndList")))       fatal("GetProcAddress(glEndList)");
  if (!(glPushMatrix    = (void *) GetProcAddress(dll, "glPushMatrix")))    fatal("GetProcAddress(glPushMatrix)");
  if (!(glPopMatrix     = (void *) GetProcAddress(dll, "glPopMatrix")))     fatal("GetProcAddress(glPopMatrix)");
  if (!(glScalef        = (void *) GetProcAddress(dll, "glScalef")))        fatal("GetProcAddress(glScalef)");
  if (!(glFlush         = (void *) GetProcAddress(dll, "glFlush")))         fatal("GetProcAddress(glFlush)");
  if (!(glClearColor    = (void *) GetProcAddress(dll, "glClearColor")))    fatal("GetProcAddress(glClearColor)");
  if (!(glClearDepth    = (void *) GetProcAddress(dll, "glClearDepth")))    fatal("GetProcAddress(glClearDepth)");
  if (!(glDepthFunc     = (void *) GetProcAddress(dll, "glDepthFunc")))     fatal("GetProcAddress(glDepthFunc)");
  if (!(glEnable        = (void *) GetProcAddress(dll, "glEnable")))        fatal("GetProcAddress(glEnable)");
  if (!(glDisable       = (void *) GetProcAddress(dll, "glDisable")))       fatal("GetProcAddress(glDisable)");
  if (!(glShadeModel    = (void *) GetProcAddress(dll, "glShadeModel")))    fatal("GetProcAddress(glShadeModel)");
  if (!(glHint          = (void *) GetProcAddress(dll, "glHint")))          fatal("GetProcAddress(glHint)");
  if (!(glMaterialfv    = (void *) GetProcAddress(dll, "glMaterialfv")))    fatal("GetProcAddress(glMaterialfv)");
  if (!(glMaterialf     = (void *) GetProcAddress(dll, "glMaterialf")))     fatal("GetProcAddress(glMaterialf)");
  if (!(glLightfv       = (void *) GetProcAddress(dll, "glLightfv")))       fatal("GetProcAddress(glLightfv)");
  if (!(glCullFace      = (void *) GetProcAddress(dll, "glCullFace")))      fatal("GetProcAddress(glCullFace)");
  if (!(glBindTexture   = (void *) GetProcAddress(dll, "glBindTexture")))   fatal("GetProcAddress(glBindTexture)");
  if (!(glGenTextures   = (void *) GetProcAddress(dll, "glGenTextures")))   fatal("GetProcAddress(glGenTextures)");
  if (!(glBlendFunc     = (void *) GetProcAddress(dll, "glBlendFunc")))     fatal("GetProcAddress(glBlendFunc)");
  if (!(glTexParameteri = (void *) GetProcAddress(dll, "glTexParameteri"))) fatal("GetProcAddress(glTexParameteri)");
  if (!(glPixelStorei   = (void *) GetProcAddress(dll, "glPixelStorei")))   fatal("GetProcAddress(glPixelStorei)");
  if (!(glTexImage2D    = (void *) GetProcAddress(dll, "glTexImage2D")))    fatal("GetProcAddress(glTexImage2D)");
  if (!(glTexEnvi       = (void *) GetProcAddress(dll, "glTexEnvi")))       fatal("GetProcAddress(glTexEnvi)");
  if (!(glTexEnvfv      = (void *) GetProcAddress(dll, "glTexEnvfv")))      fatal("GetProcAddress(glTexEnvfv)");
  if (!(glAlphaFunc     = (void *) GetProcAddress(dll, "glAlphaFunc")))     fatal("GetProcAddress(glAlphaFunc)");
  if (!(glPointSize     = (void *) GetProcAddress(dll, "glPointSize")))     fatal("GetProcAddress(glPointSize)");
//  if (!(glGetIntegerv     = (void *) GetProcAddress(dll, "glGetIntegerv")))     fatal("GetProcAddress(glGetIntegerv)");
//#define GL_DRAW_BUFFER                          0x0C01

  if (!(xwglCreateContext = (void *) GetProcAddress(dll, "wglCreateContext"))) fatal("GetProcAddress(wglCreateContext)");
  if (!(xwglDeleteContext = (void *) GetProcAddress(dll, "wglDeleteContext"))) fatal("GetProcAddress(wglDeleteContext)");
  if (!(xwglMakeCurrent   = (void *) GetProcAddress(dll, "wglMakeCurrent")))   fatal("GetProcAddress(wglMakeCurrent)");
  if (!(xwglShareLists    = (void *) GetProcAddress(dll, "wglShareLists")))    fatal("GetProcAddress(wglShareLists)");
}
#define wglCreateContext xwglCreateContext
#define wglDeleteContext xwglDeleteContext
#define wglMakeCurrent   xwglMakeCurrent
#define wglShareLists    xwglShareLists
#endif

void winset(int id)
{
  Win *w;
  for (w = winlist; w; w = w->next) if (w->id == id) goto found;
  return;
found:
  glFlush();
  wglMakeCurrent(w->hDC, w->hRC);
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
//  glFlush(); XXX

  if (curwin == w) { curwin = 0; wglMakeCurrent(w->hDC, 0); }
  else if (curwin) wglMakeCurrent(curwin->hDC, curwin->hRC);
  if (w->hRC != rootRC) wglDeleteContext(w->hRC);
  PostMessage(w->hWnd, WM_QUIT, 0, 0);
}
static DWORD WINAPI win_thread(Win *w)
{
  MSG msg;
  PIXELFORMATDESCRIPTOR pfd;
  uint PixelFormat;
  RECT rt = {0, 0, pos_w, pos_h};
  w->w = pos_w; w->h = pos_h;
  
  AdjustWindowRectEx(&rt, WS_OVERLAPPEDWINDOW, 0, WS_EX_APPWINDOW);

  if (!(w->hWnd = CreateWindowEx(WS_EX_APPWINDOW, // extended style
      "GSRV",              // class name
      "notitle",           // title
      WS_OVERLAPPEDWINDOW, // style
      pos_x, pos_y,        // position
      rt.right - rt.left,  // width
      rt.bottom - rt.top,  // height
      HWND_DESKTOP,        // parent
      0,                   // no menu
      hInstance,           // window instance
      w)))                 // user data
    fatal("CreateWindowEx()");

  if (!(w->hDC = GetDC(w->hWnd))) fatal("GetDC()");

  memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion   = 1;
  pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 16;
  pfd.iLayerType = PFD_MAIN_PLANE;

  if (!(PixelFormat = ChoosePixelFormat(w->hDC, &pfd))) fatal("ChoosePixelFormat()");
  if (!SetPixelFormat(w->hDC, PixelFormat, &pfd)) fatal("SetPixelFormat()");

  ShowWindow(w->hWnd, SW_NORMAL);
  SetEvent(w->event);

  while (GetMessage(&msg, NULL, 0, 0)) {  
    if (msg.message == WM_QUIT) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  if (debug) printf("win_thread() returning, freeing w, w->id = %d\n",w->id);
  ReleaseDC(w->hWnd, w->hDC);
  DestroyWindow(w->hWnd);
  free(w);
  return 0;
}
void winopen(int id, char *title)
{
  Win *w;
  DWORD idx;

  winclose(id);
  if (!(w = (Win *) malloc(sizeof(Win)))) fatal("malloc(Win)");
  memset(w, 0, sizeof(Win));

  w->id = id;
  w->next = winlist; 
  winlist = curwin = w;

  if (!(w->event = CreateEvent(NULL, 0, 0, NULL))) fatal("CreateEvent()");
  
  if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) win_thread, w, 0, &idx) == INVALID_HANDLE_VALUE) fatal("CreateThread()");
  if (WaitForSingleObject(w->event, INFINITE) == WAIT_FAILED) fatal("WaitForSingleObject(event)");

  if (!(w->hRC = wglCreateContext(w->hDC))) fatal("wglCreateContext()");
  if (!rootRC) {
    rootRC = w->hRC;
  } else {
    if (!wglShareLists(rootRC, w->hRC)) fatal("wglShareLists()");
  }
  if (!wglMakeCurrent(w->hDC, w->hRC)) fatal("wglMakeCurrent()");

  SetWindowText(curwin->hWnd, title);
}

void txmsg(int msg, int win, int p1, int p2)
{
  int m[4] = {msg, win, p1, p2}; // XXX shrink this?
  if (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED) fatal("WaitForSingleObject(mutex)");
  if (send(sd, (char *) m, 16, 0) != 16) { if (debug) printf("txmsg() failed quit=1\n"); quit = 1; }
  if (!ReleaseMutex(mutex)) fatal("ReleaseMutex(mutex)");
}

int rxbuf(int sd, char *p, int n) // XXX do something different!
{
  static char buf[16384]; static int len = 0; int r;
  while (len < n && len < sizeof(buf)) {
    if ((r = recv(sd, buf + len, sizeof(buf) - len, 0)) <= 0) return r;
    len += r;
  }
  if (n > len) n = len;
  memcpy(p, buf, n);
  if (len > n) memcpy(buf, buf + n, len - n);
  len -= n;
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
int rx4(void)
{
  int i;
  if (rxbuf(sd, (char *) &i, 4) != 4) { if (debug) printf("rx4() failed quit=1\n"); quit = 1; return -1; }
  nrx += 4;
  return i;
}

void rxmsg(void)
{
  union { int i; float f; } p[16]; int n;
  static char s[2048]; // XXX
  char *pixels;
  
  switch (rx4()) {
  case M_prefsize:      rx(p, 8); pos_w = p[0].i; pos_h = p[1].i; break;
  case M_prefposition:  rx(p, 8); pos_x = p[0].i; pos_y = p[1].i; break;
  case M_winopen:       rx(p, 8); rx(s, p[1].i); winopen(p[0].i, s); break;
  case M_wintitle:      rx(s, rx4()); if (curwin) SetWindowText(curwin->hWnd, s); break;
  case M_winset:        winset(rx4()); break;
  case M_winclose:      winclose(rx4()); break;
  case M_swapbuffers:   if (curwin) SwapBuffers(curwin->hDC);
                        if (debug) printf("swapbuffers() nrx=%d\n", nrx); nrx = 0; break;
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

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  RECT rt; int k;
  Win *w = (Win *) GetWindowLong(hWnd, GWL_USERDATA);

  switch (uMsg) {
  case WM_CREATE:
    w = (Win *)(((CREATESTRUCT *)lParam)->lpCreateParams);
    SetWindowLong(hWnd, GWL_USERDATA, (LONG)(w));
    break;

  case WM_CLOSE: txmsg(WINSHUT, w->id, 0, 0); break;

  case WM_PAINT:
    GetClientRect(w->hWnd, &rt);
    txmsg(REDRAW, w->id, w->w = rt.right - rt.left, w->h = rt.bottom - rt.top);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

  case WM_LBUTTONDOWN: txmsg(LDOWN, w->id, LOWORD(lParam), w->h - HIWORD(lParam)); break;
  case WM_LBUTTONUP:   txmsg(LUP,   w->id, LOWORD(lParam), w->h - HIWORD(lParam)); break;
  case WM_RBUTTONDOWN: txmsg(RDOWN, w->id, LOWORD(lParam), w->h - HIWORD(lParam)); break;
  case WM_RBUTTONUP:   txmsg(RUP,   w->id, LOWORD(lParam), w->h - HIWORD(lParam)); break;

  case WM_MOUSEMOVE:
    if (wParam == MK_LBUTTON) txmsg(LMOUSE, w->id, LOWORD(lParam), w->h - HIWORD(lParam));
    else if (wParam == MK_RBUTTON) txmsg(RMOUSE, w->id, LOWORD(lParam), w->h - HIWORD(lParam));
    break;

  case WM_CHAR: txmsg(KEY, w->id, wParam, 0); break;
  
  case WM_KEYDOWN:
    switch (wParam) {
    case VK_F1:     k = KEY_F1; break;
    case VK_F2:     k = KEY_F2; break;
    case VK_F3:     k = KEY_F3; break;
    case VK_F4:     k = KEY_F4; break;
    case VK_F5:     k = KEY_F5; break;
    case VK_F6:     k = KEY_F6; break;
    case VK_F7:     k = KEY_F7; break;
    case VK_F8:     k = KEY_F8; break;
    case VK_UP:     k = KEY_UP; break;
    case VK_DOWN:   k = KEY_DOWN; break;
    case VK_RIGHT:  k = KEY_RIGHT; break;
    case VK_LEFT:   k = KEY_LEFT; break;
    case VK_PRIOR:  k = KEY_PPAGE; break;
    case VK_NEXT:   k = KEY_NPAGE; break;
    case VK_HOME:   k = KEY_HOME; break;
    case VK_END:    k = KEY_END; break;
    case VK_INSERT: k = KEY_IC; break;
    case VK_DELETE: k = KEY_DC; break;
    default:       return 0;
    }
    if (GetKeyState(VK_SHIFT)   < 0) k |= KEY_SHIFT;
    if (GetKeyState(VK_CONTROL) < 0) k |= KEY_CTRL;
    txmsg(KEY, w->id, k, 0);
    break;

  case WM_SIZING:
    GetClientRect(w->hWnd, &rt);
    txmsg(SHAPE, w->id, w->w = rt.right - rt.left, w->h = rt.bottom - rt.top);
    break;

  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) txmsg(SHAPE, w->id, w->w = 0, w->h = 0);
    else txmsg(SHAPE, w->id, w->w = LOWORD(lParam), w->h = HIWORD(lParam));
    break;

  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  WNDCLASSEX wc; WSADATA info; int i, ld; Win *w, *n;
  char cmdline[256];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  struct sockaddr_in addr;
  int port = 5001;
  int server = 1;

  if (WSAStartup(MAKEWORD(2,0),&info) != 0) fatal("WSAStartup()");

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-v")) verbose = 1;
    else if (!strcmp(argv[i],"-I")) server = 0;
    else if (!strcmp(argv[i],"-d")) verbose = debug = 1;
    else if (!strcmp(argv[i],"-p") && i+1 < argc) port = atoi(argv[++i]);
    else fatal("usage: srv [-v] [-d] [-p port]");
  }
  
  if (server) {
    if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) fatal("socket()");
    i = 1; setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ld, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) { closesocket(ld); fatal("bind()"); }
    if (listen(ld, 1) < 0) { closesocket(ld); fatal("listen()"); }

    if (verbose) printf("Listening on port %d...\n", port) ;
    for (;;) {
      if ((sd = accept(ld, 0, 0)) < 0) fatal("accept()");
      if (verbose) printf("Accepted client connection.\n") ;
      i = 1; setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
      
      strcpy(cmdline, argv[0]);
      strcat(cmdline, " -I");
      if (debug) strcat(cmdline, " -d");
      if (verbose) strcat(cmdline, " -v");
            
      memset(&si, 0 , sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.hStdInput = (HANDLE)sd;
      si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
      si.dwFlags = STARTF_USESTDHANDLES;
      
      if (!CreateProcess(NULL, // application name
          cmdline, // command line
          NULL,    // process security attributes
          NULL,    // thread security attributes
          TRUE,    // inherit handles
          0,       // creation flags
          NULL,    // environment
          NULL,    // current directory
          &si,     // startup info
          &pi)) fatal("CreateProcess()");

      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);

      closesocket(sd);      
    }
  } else {
    sd = (SOCKET) GetStdHandle(STD_INPUT_HANDLE);
    
    hInstance = GetModuleHandle(0);

    memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC) WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GSRV";
    if (!RegisterClassEx(&wc)) fatal("RegisterClassEx()");
  
    if (!(mutex = CreateMutex(NULL, FALSE, NULL))) fatal("CreateMutex()");

#ifndef OK_OPENGL
    load_opengl();
#endif
  
    while (!quit)
      rxmsg();

    for ((w = winlist) && (n = w->next); w; (w = n) && (n = w->next)) winclose(w->id);
    if (sd >= 0) closesocket(sd);
    if (verbose) printf("Client closed.\n");
    UnregisterClass("GSRV", hInstance);
  }
  
  return 0;
}
