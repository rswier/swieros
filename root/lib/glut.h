// glut.h

// single window only

enum { GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON };
enum { GLUT_DOWN, GLUT_UP };

void (*_display)();
void (*_idle)();
void (*_reshape)();
void (*_keyboard)();
void (*_special)();
void (*_motion)();
void (*_mouse)();

void glutInit(int *argcp, char **argv)
{
  while (*argcp > 1) {
    if (!strcmp(*argv,"-host")) { --*argcp; ghost(*++argv); }
    else if (!strcmp(*argv,"-port")) { --*argcp; gport(atoi(*++argv)); }
    --*argcp; ++argv; 
  }
  ginit();
}
void glutInitWindowSize(int w, int h) { prefsize(w, h); }
void glutInitWindowPosition(int x, int y) { prefposition(x, y); }
int glutCreateWindow(char *title) { return winopen(title); }
void glutDisplayFunc(void (*f)()) { _display = f; }
void glutIdleFunc(void (*f)()) { _idle = f; }
void glutReshapeFunc(void (*f)()) { _reshape = f; }
void glutKeyboardFunc(void (*f)()) { _keyboard = f; }
void glutSpecialFunc(void (*f)()) { _special = f; }
void glutMotionFunc(void (*f)()) { _motion = f; }
void glutMouseFunc(void (*f)()) { _mouse = f; }
void glutPostRedisplay(void) { if (_display) (*_display)(); }
void glutSwapBuffers(void) { swapbuffers(); }
void glutMainLoop(void)
{
  int w, x, y;
  for (;;) {
    while (qtest()) {
      switch (qread(&w, &x, &y)) {
      case WINSHUT: winclose(w);
      case HANGUP:  return;
      case SHAPE:   if (_reshape) (*_reshape)(x, y);
      case REDRAW:  if (_display) (*_display)(); break;
      case KEY:     if (x < 256) { if (_keyboard) (*_keyboard)(x, 0, 0); } else { if (_special) (*_special)(x, 0, 0); } break;
      case LDOWN:   if (_mouse) (*_mouse)(GLUT_LEFT_BUTTON,  GLUT_DOWN, x, y); break;
      case LUP:     if (_mouse) (*_mouse)(GLUT_LEFT_BUTTON,  GLUT_UP,   x, y); break;
      case RDOWN:   if (_mouse) (*_mouse)(GLUT_RIGHT_BUTTON, GLUT_DOWN, x, y); break;
      case RUP:     if (_mouse) (*_mouse)(GLUT_RIGHT_BUTTON, GLUT_UP,   x, y); break;
      case LMOUSE:
      case RMOUSE:  if (_motion) (*_motion)(x, y);
      }
    }
    if (_idle) (*_idle)();
  }
}
