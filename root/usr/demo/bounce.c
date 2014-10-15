// bounce -- Bouncing ball demo
//
// Usage:  bounce
//
// Originally by Brian Paul

#include <u.h>
#include <libc.h>
#include <net.h>
#include <gl.h>
#include <libm.h>

double Cos(double X) { return cos( (X) * 3.14159/180.0 ); }
double Sin(double X) { return sin( (X) * 3.14159/180.0 ); }


GLuint ball;
GLenum Mode;
GLfloat Zrot = 0.0, Zstep = 6.0;
GLfloat Xpos = 0.0, Ypos = 1.0;
GLfloat Xvel = 0.2, Yvel = 0.0;
GLfloat Xmin=-4.0, Xmax=4.0;
GLfloat Ymin=-3.8, Ymax=4.0;
GLfloat G = -0.1;

GLuint make_ball( void )
{
  GLuint list;
  GLfloat a, b;
  GLfloat da = 18.0, db = 18.0;
  GLfloat radius = 1.0;
  GLuint color;
  GLfloat x, y, z;

  static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
  static GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

  list = glGenLists( 1 );

  glNewList( list, GL_COMPILE );

  color = 0;
  for (a=-90.0;a+da<=90.0;a+=da) {
    glBegin( GL_QUAD_STRIP );
    for (b=0.0;b<=360.0;b+=db) {
      if (color)
        glColor4fv(red);    // TK_SETCOLOR( Mode, TK_RED );
      else
        glColor4fv(white);  // TK_SETCOLOR( Mode, TK_WHITE );

      x = Cos(b) * Cos(a);
      y = Sin(b) * Cos(a);
      z = Sin(a);
      glVertex3f( x, y, z );

      x = radius * Cos(b) * Cos(a+da);
      y = radius * Sin(b) * Cos(a+da);
      z = radius * Sin(a+da);
      glVertex3f( x, y, z );

      color = 1-color;
    }
    glEnd();
  }
  glEndList();
  return list;
}

void reshape(int width, int height)
{
  if (width == 0 || height == 0) return;
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-6.0, 6.0, -6.0, 6.0, -6.0, 6.0);
  glMatrixMode(GL_MODELVIEW);
}

void idle(void)
{
  static float vel0 = -100.0;

  Zrot += Zstep;
  Xpos += Xvel;

  if (Xpos>=Xmax) {
    Xpos = Xmax;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }
  if (Xpos<=Xmin) {
    Xpos = Xmin;
    Xvel = -Xvel;
    Zstep = -Zstep;
  }

  Ypos += Yvel;
  Yvel += G;
  if (Ypos<Ymin) {
    Ypos = Ymin;
    if (vel0==-100.0)  vel0 = fabs(Yvel);
    Yvel = vel0;
  }
}


void draw(void)
{
  GLint i, j;
  static GLfloat cyan[4] = { 0.0, 1.0, 1.0, 1.0 };

  glClear( GL_COLOR_BUFFER_BIT );

  glColor4fv(cyan);    // TK_SETCOLOR( Mode, TK_CYAN );
  glBegin( GL_LINES );
  for (i=-5;i<=5;i++) {
    glVertex2f( i, -5 );
    glVertex2f( i, 5 );
  }
  for (i=-5;i<=5;i++) {
    glVertex2f( -5,i );
    glVertex2f( 5,i );
  }
  for (i=-5;i<=5;i++) {
    glVertex2f( i, -5 );
    glVertex2f( i*1.15, -5.9 );
  }
  glVertex2f( -5.3, -5.35 );
  glVertex2f( 5.3, -5.35 );
  glVertex2f( -5.75, -5.9 );
  glVertex2f( 5.75, -5.9 );
  glEnd();


  glPushMatrix();
  glTranslatef( Xpos, Ypos, 0.0 );
  glScalef( 2.0, 2.0, 2.0 );
  glRotatef( 8.0, 0.0, 0.0, 1.0 );
  glRotatef( 90.0, 1.0, 0.0, 0.0 );
  glRotatef( Zrot, 0.0, 0.0, 1.0 );

  glCallList( ball );

  glPopMatrix();

//   glFlush();   XXX
}

void init(void)
{
  ball = make_ball();

  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
//  glDisable(GL_DITHER);
  glShadeModel(GL_FLAT);
}

void event_loop()
{
  int w,x,y;
  for (;;) { 
//    Sleep(5);
    while (qtest()) {
      switch (qread(&w, &x, &y)) {
      case REDRAW:
      case SHAPE:
        reshape(x, y);
        break;
      case KEY:
        switch (x) {
        case 'q': case '\e': winclose(w); return;
        }
        break;
      case WINSHUT:
        winclose(w);
        return;
      case HANGUP:
        dprintf(2,"hangup\n");
        exit(9);
      }
    }
    idle();
    draw();
    swapbuffers();
  }
}


int main(int argc, char *argv[])
{
  while (--argc) {
    argv++;
    if (!strcmp(*argv,"-host") && argc > 1) { argc--; ghost(*++argv); }
    else if (!strcmp(*argv,"-port") && argc > 1) { argc--; gport(atoi(*++argv)); }
  }
  ginit();
  prefsize(300, 300);
  winopen("Bounce");
  init();
  event_loop();
  gexit();   
  return 0;
}
