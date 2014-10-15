// tris.c

#include <u.h>
#include <libc.h>
#include <libm.h>
#include <net.h>
#include <gl.h>

void gluPerspectivef(GLfloat fovy, GLfloat aspect, GLfloat n, GLfloat f)
{
  GLdouble ymax = n * tan( fovy * M_PI / 360.0 );
  glFrustum(-ymax * aspect, ymax * aspect, -ymax, ymax, n, f);
}

void draw(void)
{
  static GLdouble angle = 0.0;
  int rot1, rot2;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -6.0);
  glRotatef(angle, 0.0, 1.0, 0.0);

  for (rot1=0; rot1<2; rot1++) {
    glRotatef(90.0,  0.0, 1.0, 0.0);
    glRotatef(180.0, 1.0, 0.0, 0.0);
    for (rot2=0; rot2<2; rot2++) {
      glRotatef(180.0, 0.0, 1.0, 0.0);
      glBegin(GL_TRIANGLES);
      glColor4f(1.0, 0.0, 0.0, 1.0);  glVertex3f( 0.0,  1.0, 0.0); // red
      glColor4f(0.0, 0.5, 0.0, 1.0);  glVertex3f(-1.0, -1.0, 1.0); // green
      glColor4f(0.0, 0.0, 1.0, 1.0);  glVertex3f( 1.0, -1.0, 1.0); // blue
      glEnd();
    }
  }
  swapbuffers();
  angle += 1.0;
}

int main(int argc, char *argv[])
{
  int i,j,e,w,x,y,w1,w2;
  char ss[100];
  
  while (--argc) {
    argv++;
    if (!strcmp(*argv,"-host") && argc > 1) { argc--; ghost(*++argv); }
//    else if (!strcmp(*argv,"-port") && argc > 1) { argc--; gport(atoi(*++argv)); }  XXX
  }
  ginit();
  
  prefsize(640,480);
  prefposition(0,0);
  w1 = winopen("window1");
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  prefposition(650,0);
  w2 = winopen("window2");
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  
  for (i=0; i<100000; i++) {
//    Sleep(10);
//    read(0,ss,1);
//    printf("%c\n",ss[0]);

    
    while (qtest()) {
//    if (qtest())
//    {
      switch (e = qread(&w, &x, &y)) {
      case REDRAW:
        dprintf(2,"got draw %d %d %d\n",w,x,y);
        goto hop;
      case SHAPE:
        dprintf(2,"got shape %d %d %d\n",w,x,y);
hop:    winset(w);
        glViewport(0, 0, x, y);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if (y) gluPerspectivef(45.0, (GLdouble)x / y, 1.0, 100.0);
        glMatrixMode(GL_MODELVIEW);
        break;
        
      case WINSHUT:
        dprintf(2,"got winshut %d %d %d\n",w,x,y);
        winclose(w);
        if (w == w1) w1 = -1;
        else if (w == w2) w2 = -1;
        break;
      
      case KEY:
        dprintf(2,"bail\n"); gexit(); exit(0);

      case HANGUP:
        dprintf(2,"hangup\n"); exit(1);
      default:
        dprintf(2,"unknown %d %d %d %d\n",e,w,x,y); //exit(1);
      }
    }

    if (w1 >= 0) {
      winset(w1);
      sprintf(ss,"window1 : %d",i);
      wintitle(ss);
      draw();
    }
    if (w2 >= 0) {
      winset(w2);
      sprintf(ss,"window2 : %d",i);
      wintitle(ss);
      draw();
    }
    if (w1 < 0 && w2 < 0) break;
  }
  dprintf(2,"bye\n");
  gexit();
  return 0;
}
