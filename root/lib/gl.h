// gl.h - send OpenGL commands to a remote display server     XXX fix flow control/responsiveness issues

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

  GL_FALSE = 0, GL_TRUE,
  GL_ZERO = 0, GL_ONE,
  GL_POINTS = 0, GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS, GL_QUAD_STRIP, GL_POLYGON,
  GL_NEVER = 0x0200, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS,
  GL_SRC_COLOR = 0x0300, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
  GL_FRONT_LEFT = 0x0400, GL_FRONT_RIGHT, GL_BACK_LEFT, GL_BACK_RIGHT, GL_FRONT, GL_BACK, GL_LEFT, GL_RIGHT, GL_FRONT_AND_BACK,
  GL_POINT_SMOOTH = 0x0B10, GL_POINT_SIZE,
  GL_LINE_SMOOTH = 0x0B20, GL_LINE_WIDTH,
  GL_CULL_FACE = 0x0B44,
  GL_LIGHTING = 0x0B50,
  GL_COLOR_MATERIAL = 0x0B57,
  GL_DEPTH_TEST = 0x0B71,
  GL_NORMALIZE = 0x0BA1,
  GL_BLEND = 0x0BE2,  
  GL_PERSPECTIVE_CORRECTION_HINT = 0x0C50, GL_POINT_SMOOTH_HINT, GL_LINE_SMOOTH_HINT, GL_POLYGON_SMOOTH_HINT, GL_FOG_HINT,
  GL_UNPACK_ALIGNMENT = 0x0CF5,
  GL_TEXTURE_2D = 0x0DE1,
  GL_DONT_CARE = 0x1100, GL_FASTEST, GL_NICEST,
  GL_AMBIENT = 0x1200, GL_DIFFUSE, GL_SPECULAR, GL_POSITION,  
  GL_COMPILE = 0x1300,
  GL_BYTE = 0x1400, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT, GL_DOUBLE = 0x140A, 
  GL_EMISSION = 0x1600, GL_SHININESS, GL_AMBIENT_AND_DIFFUSE,
  GL_MODELVIEW = 0x1700, GL_PROJECTION, GL_TEXTURE,
  GL_RGBA = 0x1908,
  GL_FLAT = 0x1D00, GL_SMOOTH,
  GL_DECAL = 0x2101,
  GL_TEXTURE_ENV_MODE = 0x2200, GL_TEXTURE_ENV_COLOR,
  GL_TEXTURE_ENV = 0x2300,
  GL_LINEAR = 0x2601,  // XXX
  GL_TEXTURE_MAG_FILTER = 0x2800, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,  
  GL_REPEAT = 0x2901,
  GL_LIGHT0 = 0x4000,
  GL_DEPTH_BUFFER_BIT = 0x0100,
  GL_COLOR_BUFFER_BIT = 0x4000,
};

typedef uchar  GLubyte;
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

char _ghost[50] = "127.0.0.1";
int _gport = 5001;
int _gd = -1;
union { int i; float f; } _gbuf[1024], *_gp, *_gmax;

void ghost(char *s)
{
  strcpy(_ghost, s);
}
void gport(int n)
{
  _gport = n;
}
int ginit(void)
{
  struct sockaddr_in addr; struct hostent *hp;
  _gmax = (_gp = _gbuf) + 1022;
  if ((_gd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
//  hp = gethostbyname(_ghost);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_gport);
//  memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
  addr.sin_addr.s_addr = htonl(0x7f000001); // XXX hardcoded for now
//  addr.sin_addr.s_addr = htonl((10<<24)|(1<<16)|(10<<8)|73);
//  addr.sin_addr.s_addr = htonl((192<<24)|(168<<16)|(30<<8)|129);
  if(connect(_gd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) { close(_gd); _gd = -1; return -2; }
  return _gd;
}
void gwrite(void *p, int n)
{
  int r;
  if (_gd >= 0) {
    while (n) {
      if ((r = write(_gd, p, n)) < 0) { close(_gd); _gd = -1; return; }
      n -= r;
      p += r;
    }
  }
}
void gflush(void)
{
  if (_gp > _gbuf) {
    gwrite(_gbuf, (int)_gp - (int)_gbuf);
    _gp = _gbuf;
  }
}
int qtest(void)
{
  struct pollfd pfd;
  if (_gd < 0) return -1;
  pfd.fd = _gd;
  pfd.events = POLLIN;
  return poll(&pfd, 1, 0);
}
int qread(int *win, int *p1, int *p2)
{
  int d[4];
  if (_gd < 0) return HANGUP;
  if (read(_gd, (char *) d, sizeof(d)) != sizeof(d)) { close(_gd); _gd = -1; return HANGUP; }
  *win = d[1];
  *p1 = d[2];
  *p2 = d[3];
  return d[0];
}
int winopen(char *s)
{
  static int winids;
  int id = winids++, slen = (strlen(s) + 4) & -4;
  if (slen > 2048) { s = "Title Too Long!"; slen = 16; }
  if (_gp + 3 + (slen>>2) > _gmax) gflush();
  _gp->i = M_winopen;
  _gp[1].i = id;
  _gp[2].i = slen;
  strcpy((char *)(_gp + 3), s);
  _gp += 3 + (slen>>2);
  gflush();
  return id;
}
void wintitle(char *s)
{
  int slen = (strlen(s) + 4) & -4;
  if (slen > 2048) { s = "Title Too Long!"; slen = 16; }
  if (_gp + 2 + (slen>>2) > _gmax) gflush();
  _gp->i = M_wintitle;
  _gp[1].i = slen;
  strcpy((char *)(_gp + 2), s);
  _gp += 2 + (slen>>2);
}
void gexit(void)
{
  gflush();
  if (_gd >= 0) { close(_gd); _gd = -1; }
}
void winset(int a) { _gp->i = M_winset; _gp[1].i = a; if ((_gp += 2) > _gmax) gflush(); }
void winclose(int a) { _gp->i = M_winclose; _gp[1].i = a; if ((_gp += 2) > _gmax) gflush(); }
void prefsize(int w, int h) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_prefsize; _gp[1].i = w; _gp[2].i = h; _gp += 3; }
void prefposition(int x, int y) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_prefposition; _gp[1].i = x; _gp[2].i = y; _gp += 3; }
void swapbuffers(void) { _gp->i = M_swapbuffers; _gp++; gflush(); }

void glFlush(void) { _gp->i = M_Flush; _gp++; gflush(); }
void glCallLists(GLsizei n, GLenum t, GLvoid *s) // XXX only works for GL_UNSIGNED_BYTE
{
  if (n > 2044) return;
  if (_gp + 3 + ((n+3)>>2) > _gmax) gflush();
  _gp->i = M_CallLists;
  _gp[1].i = n;
  memcpy(_gp + 2, s, n);
  _gp += 2 + ((n+3)>>2);
}
void glCallList(GLuint n) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_CallLists; _gp[1].i = 1; _gp[2].i = n; _gp += 3; }
void glListBase(GLuint base) { _gp->i = M_ListBase; _gp[1].i = base; if ((_gp += 2) > _gmax) gflush(); }
void glLightfv(GLenum light, GLenum pname, GLfloat *params)
{
  if (_gp + 7 > _gmax) gflush();
  _gp->i = M_Lightfv;
  _gp[1].i = light;
  _gp[2].i = pname;
  _gp[3].f = params[0];
  _gp[4].f = params[1];
  _gp[5].f = params[2];
  _gp[6].f = params[3];
  _gp += 7;
}

void glBegin(GLenum mode) { _gp->i = M_Begin; _gp[1].i = mode; if ((_gp += 2) > _gmax) gflush(); }
void glEnd(void) { _gp->i = M_End; if (++_gp > _gmax) gflush(); }
void glPushMatrix(void) { _gp->i = M_PushMatrix; if (++_gp > _gmax) gflush(); }
void glPopMatrix(void) { _gp->i = M_PopMatrix; if (++_gp > _gmax) gflush(); }
void glLoadIdentity(void) { _gp->i = M_LoadIdentity; if (++_gp > _gmax) gflush(); }
void glClear(GLbitfield mask) { _gp->i = M_Clear; _gp[1].i = mask; if ((_gp += 2) > _gmax) gflush(); }
void glEnable(GLenum cap) { _gp->i = M_Enable; _gp[1].i = cap; if ((_gp += 2) > _gmax) gflush(); }
void glDisable(GLenum cap) { _gp->i = M_Disable; _gp[1].i = cap; if ((_gp += 2) > _gmax) gflush(); }
void glCullFace(GLenum face) { _gp->i = M_CullFace; _gp[1].i = face; if ((_gp += 2) > _gmax) gflush(); }
void glMatrixMode(GLenum mode) { _gp->i = M_MatrixMode; _gp[1].i = mode; if ((_gp += 2) > _gmax) gflush(); }
void glEndList(void) { _gp->i = M_EndList; if (++_gp > _gmax) gflush(); }
void glShadeModel(GLenum mode) { _gp->i = M_ShadeModel; _gp[1].i = mode; if ((_gp += 2) > _gmax) gflush(); }
void glColor3f(GLfloat r, GLfloat g, GLfloat b) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Color; _gp[1].f = r; _gp[2].f = g; _gp[3].f = b; _gp[4].f = 1.0; _gp += 5; }
void glColor3fv(GLfloat *v) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Color; _gp[1].f = v[0]; _gp[2].f = v[1]; _gp[3].f = v[2]; _gp[4].f = 1.0; _gp += 5; }
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Color; _gp[1].f = r; _gp[2].f = g; _gp[3].f = b; _gp[4].f = a; _gp += 5; }
void glColor4fv(GLfloat *v) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Color; _gp[1].f = v[0]; _gp[2].f = v[1]; _gp[3].f = v[2]; _gp[4].f = v[3]; _gp += 5; }
void glColor4ubv(GLubyte *v) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Color; _gp[1].f = v[3]/255.0; _gp[2].f = v[2]/255.0; _gp[3].f = v[1]/255.0; _gp[4].f = v[0]/255.0; _gp += 5; }
uint glGenLists(GLsizei range) { static int listid = 1; listid += range; return listid - range; }
void glNewList(GLuint list, GLenum mode) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_NewList; _gp[1].i = list; _gp[2].i = mode; _gp += 3; }
void glClearColor(GLclampf a, GLclampf b, GLclampf c, GLclampf d) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_ClearColor; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp[4].f = d; _gp += 5; }
void glClearDepth(GLclampd depth) { _gp->i = M_ClearDepth; _gp[1].f = depth; if ((_gp += 2) > _gmax) gflush(); }
void glDepthFunc(GLenum func) { _gp->i = M_DepthFunc; _gp[1].i = func; if ((_gp += 2) > _gmax) gflush(); }
void glHint(GLenum target, GLenum mode) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_Hint; _gp[1].i = target; _gp[2].i = mode; _gp += 3; }
void glTranslatef(GLfloat a, GLfloat b, GLfloat c) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Translatef; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp += 4; }
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Vertex; _gp[1].f = x; _gp[2].f = y; _gp[3].f = z; _gp += 4; }
void glVertex3fv(GLfloat *v) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Vertex; _gp[1].f = v[0]; _gp[2].f = v[1]; _gp[3].f = v[2]; _gp += 4; }
void glVertex2f(GLfloat x, GLfloat y) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Vertex; _gp[1].f = x; _gp[2].f = y; _gp[3].f = 0.0; _gp += 4; }
void glVertex2fv(GLfloat *v) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Vertex; _gp[1].f = v[0]; _gp[2].f = v[1]; _gp[3].f = 0.0; _gp += 4; }
void glNormal3f(GLfloat a, GLfloat b, GLfloat c) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Normal; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp += 4; }
void glNormal3fv(GLfloat *v) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Normal; _gp[1].f = v[0]; _gp[2].f = v[1]; _gp[3].f = v[2]; _gp += 4; }
void glScalef(GLfloat a, GLfloat b, GLfloat c) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Scalef; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp += 4; }
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Viewport; _gp[1].i = x; _gp[2].i = y; _gp[3].i = width; _gp[4].i = height; _gp += 5; }
void glRotatef(GLfloat a, GLfloat b, GLfloat c, GLfloat d) { if (_gp + 5 > _gmax) gflush(); _gp->i = M_Rotatef; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp[4].f = d; _gp += 5; }
void glFrustum(GLdouble a, GLdouble b, GLdouble c, GLdouble d, GLdouble e, GLdouble f) { if (_gp + 7 > _gmax) gflush(); _gp->i = M_Frustum; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp[4].f = d; _gp[5].f = e; _gp[6].f = f; _gp += 7; }
void glOrtho(GLdouble a, GLdouble b, GLdouble c, GLdouble d, GLdouble e, GLdouble f) { if (_gp + 7 > _gmax) gflush(); _gp->i = M_Ortho; _gp[1].f = a; _gp[2].f = b; _gp[3].f = c; _gp[4].f = d; _gp[5].f = e; _gp[6].f = f; _gp += 7; }
void glLoadMatrixf(GLfloat *m)
{
  if (_gp + 17 > _gmax) gflush();
  _gp->i = M_LoadMatrixf;
  _gp[1].f  = m[0];  _gp[2].f  = m[1];  _gp[3].f  = m[2];  _gp[4].f  = m[3];
  _gp[5].f  = m[4];  _gp[6].f  = m[5];  _gp[7].f  = m[6];  _gp[8].f  = m[7];
  _gp[9].f  = m[8];  _gp[10].f = m[9];  _gp[11].f = m[10]; _gp[12].f = m[11];
  _gp[13].f = m[12]; _gp[14].f = m[13]; _gp[15].f = m[14]; _gp[16].f = m[15];
  _gp += 17;
}
void glMultMatrixf(GLfloat *m)
{
  if (_gp + 17 > _gmax) gflush();
  _gp->i = M_MultMatrixf;
  _gp[1].f  = m[0];  _gp[2].f  = m[1];  _gp[3].f  = m[2];  _gp[4].f  = m[3];
  _gp[5].f  = m[4];  _gp[6].f  = m[5];  _gp[7].f  = m[6];  _gp[8].f  = m[7];
  _gp[9].f  = m[8];  _gp[10].f = m[9];  _gp[11].f = m[10]; _gp[12].f = m[11];
  _gp[13].f = m[12]; _gp[14].f = m[13]; _gp[15].f = m[14]; _gp[16].f = m[15];
  _gp += 17;
}

void glTexCoord2f(GLfloat a, GLfloat b) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_TexCoord; _gp[1].f = a; _gp[2].f = b; _gp += 3; }  // XXX remove!!!!

void glBindTexture(GLenum target, GLuint texture) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_BindTexture; _gp[1].i = target; _gp[2].i = texture; _gp += 3; }
void glGenTextures(GLsizei n, GLuint *textures) { static int texid = 1; while (n--) *textures++ = texid++; }
void glBlendFunc(GLenum sfactor, GLenum dfactor) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_BlendFunc; _gp[1].i = sfactor; _gp[2].i = dfactor; _gp += 3; }
void glTexParameteri(GLenum target, GLenum pname, GLint param) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_TexParameteri; _gp[1].i = target; _gp[2].i = pname; _gp[3].i = param; _gp += 4; }
void glPixelStorei(GLenum pname, GLint param) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_PixelStorei; _gp[1].i = pname; _gp[2].i = param; _gp += 3; }

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid *pixels)
{
  int n;
  if (internalFormat != GL_RGBA || format != GL_RGBA || type != GL_UNSIGNED_BYTE) return;
  if (_gp + 9 > _gmax) gflush();
  _gp->i = M_TexImage2D;
  _gp[1].i = target;
  _gp[2].i = level;
  _gp[3].i = internalFormat;
  _gp[4].i = width;
  _gp[5].i = height;
  _gp[6].i = border;
  _gp[7].i = format;
  _gp[8].i = type;
  _gp += 9;
  gflush();
  n = width * height * 4;
  gwrite(pixels, n);  // XXX compress
}

void glMaterialf(GLenum face, GLenum pname, GLfloat p) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_Materialf; _gp[1].i = face; _gp[2].i = pname; _gp[3].f = p; _gp += 4; }
void glMaterialfv(GLenum face, GLenum pname, GLfloat *v) { if (_gp + 7 > _gmax) gflush(); _gp->i = M_Materialfv; _gp[1].i = face; _gp[2].i = pname; _gp[3].f = v[0]; _gp[4].f = v[1]; _gp[5].f = v[2]; _gp[6].f = v[3]; _gp += 7; }
void glTexEnvi(GLenum target, GLenum pname, GLint param) { if (_gp + 4 > _gmax) gflush(); _gp->i = M_TexEnvi; _gp[1].i = target; _gp[2].i = pname; _gp[3].i = param; _gp += 4; }
void glTexEnvfv(GLenum target, GLenum pname, GLfloat *v) { if (_gp + 7 > _gmax) gflush(); _gp->i = M_TexEnvfv; _gp[1].i = target; _gp[2].i = pname; _gp[3].f = v[0]; _gp[4].f = v[1]; _gp[5].f = v[2]; _gp[6].f = v[3]; _gp += 7; }
void glAlphaFunc(GLenum func, GLclampf ref) { if (_gp + 3 > _gmax) gflush(); _gp->i = M_AlphaFunc; _gp[1].i = func; _gp[2].f = ref; _gp += 3; }
void glPointSize(GLfloat size) { _gp->i = M_PointSize; _gp[1].f = size; if ((_gp += 2) > _gmax) gflush(); }

/* XXX Todo:
  fix flow control/responsiveness issues (in conjuction with server)
  simple compression scheme (RLE?) for texture downloading

  void glClearStencil (GLint s);
  void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
  void glDepthMask (GLboolean flag);
  void glDepthRange (GLclampd zNear, GLclampd zFar);
  void glFinish (void);
  void glFrontFace (GLenum mode);
  void glLightModelfv (GLenum pname, const GLfloat *params);
  void glLineWidth (GLfloat width);
  void glMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t);
  void glMultiTexCoord2fv (GLenum target, const GLfloat *v);
  void glPolygonOffset (GLfloat factor, GLfloat units);
  void glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
  void glStencilFunc (GLenum func, GLint ref, GLuint mask);
  void glStencilMask (GLuint mask);
  void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);
  void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
  compression, see bak31
*/
