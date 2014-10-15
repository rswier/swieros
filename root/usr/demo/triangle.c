// triangle.c

#include <u.h>
#include <libc.h>
#include <libm.h>
#include <net.h>
#include <gl.h>
#include <glut.h>

init()
{
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);
  glClearColor(0.0, 0.0, 0.0, 0.0);
}

reshape(int width, int height)
{
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-2.0, 2.0, -2.0, 2.0, 6.0, 20.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -8.0);
}

display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(-1.0, 0.0, 0.0);
  glRotatef(0.0, 0.0, 0.0, 1.0);
  glBegin(GL_TRIANGLES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex2f(0.0, -0.5);
    glColor3f(0.0, 1.0, 0.0);
    glVertex2f(1.0, 1.0);
    glColor3f(0.0, 0.0, 1.0);
    glVertex2f(-1.0, 1.0);
  glEnd();
  glPopMatrix();
  glutSwapBuffers();
}

idle()
{
  glutPostRedisplay();
}

int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  glutInitWindowSize(320, 240);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("OpenGL triangle");
  init();
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutMainLoop();
  return 0;
}
