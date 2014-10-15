// asteroids.c

/* Copyright (c) Mark J. Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <u.h>
#include <libc.h>
#include <net.h>
#include <gl.h>
#include <libm.h>

float angle = 0.0;
int left, right;
int leftTime, rightTime;
int thrust, thrustTime;
int joyThrust = 0, joyLeft = 0, joyRight = 0;
float x = 20, y = 20, xv, yv, v;
int shield = 0, joyShield = 0;
int lastTime;
int resuming = 1;

typedef struct {
  int inuse;

  float x;
  float y;

  float v;
  float xv;
  float yv;

  int expire;
} Bullet;

enum { MAX_BULLETS = 10 };

Bullet bullet[MAX_BULLETS];

int allocBullet(void)
{
  int i;

  for (i=0; i<MAX_BULLETS; i++) {
    if (!bullet[i].inuse) {
      return i;
    }
  }
  return -1;
}

void initBullet(int i, int time)
{
  float c, s;
  c = cos(angle*M_PI/180.0);
  s = sin(angle*M_PI/180.0);

  bullet[i].inuse = 1;
  bullet[i].x = x + 2 * c;
  bullet[i].y = y + 2 * s;
  bullet[i].v = 0.025;
  bullet[i].xv = xv + c * bullet[i].v;
  bullet[i].yv = yv + s * bullet[i].v;
  bullet[i].expire = time + 1000;
}

void advanceBullets(int delta, int time)
{
  int i;
  float x, y;

  for (i=0; i<MAX_BULLETS; i++) {
    if (bullet[i].inuse) {

      if (time > bullet[i].expire) {
        bullet[i].inuse = 0;
        continue;
      }
      x = bullet[i].x + bullet[i].xv * delta;
      y = bullet[i].y + bullet[i].yv * delta;
      x = x / 40.0;
      bullet[i].x = (x - floor(x))*40.0;
      y = y / 40.0;
      bullet[i].y = (y - floor(y))*40.0;
    }
  }
}

void shotBullet(void)
{
  int entry;

  entry = allocBullet();
  if (entry >= 0) {
//    initBullet(entry, glutGet(GLUT_ELAPSED_TIME));
    initBullet(entry, lastTime);
  }
}

void drawBullets(void)
{
  int i;

  glBegin(GL_POINTS);
  glColor4f(1.0, 0.0, 1.0, 1.0);
  for (i=0; i<MAX_BULLETS; i++) {
    if (bullet[i].inuse) {
      glVertex2f(bullet[i].x, bullet[i].y);
    }
  }
  glEnd();
}

void drawShip(float angle)
{
  float rad;

  glPushMatrix();
  glTranslatef(x, y, 0.0);
  glRotatef(angle, 0.0, 0.0, 1.0);
  if (thrust) {
    glColor4f(1.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.75, -0.5);
    glVertex2f(-1.75, 0);
    glVertex2f(-0.75, 0.5);
    glEnd();
  }
  glColor4f(1.0, 1.0, 0.0, 1.0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(2.0, 0.0);
  glVertex2f(-1.0, -1.0);
  glVertex2f(-0.5, 0.0);
  glVertex2f(-1.0, 1.0);
  glVertex2f(2.0, 0.0);
  glEnd();
  if (shield) {
    glColor4f(0.1, 0.1, 1.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for (rad=0.0; rad<12.0; rad += 1.0) {
      glVertex2f(2.3 * cos(2*rad/M_PI)+0.2, 2.0 * sin(2*rad/M_PI));
    }
    glEnd();
  }
  glPopMatrix();
}

void draw(void)
{
  glClear(GL_COLOR_BUFFER_BIT);
  drawShip(angle);
  drawBullets();
}

void idle(void)
{
  int time, delta;

  time = lastTime + 10; // XXX  = glutGet(GLUT_ELAPSED_TIME);
  if (resuming) {
    time = lastTime = 0; // lastTime = time;
    resuming = 0;
  }
  if (left) {
    delta = time - leftTime;
    angle = angle + delta * 0.4;
    leftTime = time;
  }
  if (right) {
    delta = time - rightTime;
    angle = angle - delta * 0.4;
    rightTime = time;
  }
  if (thrust) {
    delta = time - thrustTime;
    v = delta * 0.00004;
    xv = xv + cos(angle*M_PI/180.0) * v;
    yv = yv + sin(angle*M_PI/180.0) * v;
    thrustTime = time;
  }
  delta = time - lastTime;
  x = x + xv * delta;
  y = y + yv * delta;
  x = x / 40.0;
  x = (x - floor(x))*40.0;
  y = y / 40.0;
  y = (y - floor(y))*40.0;
  lastTime = time;
  advanceBullets(delta, time);

  thrust = shield = left = right = 0; // XXX should be on keyup
}

void key(char key)
{
  switch (key) {
  case 'q':
    exit(0);
    break;
  case 'k':
    thrust = 1;
    thrustTime = lastTime; //glutGet(GLUT_ELAPSED_TIME);
    break;
  case 'c':
    shield = 1;
    break;
  case 'z':
    x = 20;
    y = 20;
    xv = 0;
    yv = 0;
    break;
  case 'l':
    shotBullet();
    break;
  case 't':
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    break;
  case 'y':
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    break;
  case 'a':
    left = 1;
    leftTime = lastTime; // glutGet(GLUT_ELAPSED_TIME);
    break;
  case 's':
    right = 1;
    rightTime = lastTime; //glutGet(GLUT_ELAPSED_TIME);
    break;
  }
}

void event_loop(void)
{
  int e,w,x,y;
  for (;;) {
    while (qtest()) {
      e = qread(&w, &x, &y);
      switch (e) {
      case KEY:
         key(x);
         break;
               
      case REDRAW:
      case SHAPE:
        break;
        
      case WINSHUT:
      case HANGUP:
        return; 
      }
    }
    idle();
    draw();
    swapbuffers();
  }
}

void init(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 40.0, 0.0, 40.0, 0.0, 40.0);
  glMatrixMode(GL_MODELVIEW); 
  glPointSize(3.0);
}

int main(int argc, char *argv[])
{
  ginit();
  prefsize(640, 480);
  winopen("Asteroids");
  init();
  event_loop();
  gexit();
  return 0;
}
