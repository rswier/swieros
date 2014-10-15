// vt -- terminal emulator
//
// Usage:  vt [-host n] [-port n] [-t title] [-e command]

#include <u.h>
#include <libc.h>
#include <net.h>
#include <gl.h>
#include <font.h>
#include <forms.h>

uint pal[256] = { FL_BLACK, FL_MAROON, FL_GREEN, FL_OLIVE,  FL_NAVY, FL_PURPLE,  FL_TEAL, FL_SILVER,
                  FL_GRAY,  FL_RED,    FL_LIME,  FL_YELLOW, FL_BLUE, FL_FUCHSIA, FL_AQUA, FL_WHITE };
FL_FORM *form;
int x, y; // cursor
int p1[2], p2[2]; // pipes
uchar crt[25*80], fga[25*80], bga[25*80]; // character, foreground, background arrays     XXX max 1024 * 220  (220 x 220 visible)
uint hidecursor, trackmouse, def_bg, bg, def_fg, fg, reverse, underline, bold; // terminal and character attributes     XXX set def_fg,def_bg
char *ep; // escape argument pointer
int debug;

void clr(int i, int n)
{
  memset(&crt[i], 0, n);
  memset(&bga[i], def_bg, n);
}

int arg(int v)
{
  if ('0' <= *ep && *ep <= '9') {
    v = *ep++ - '0';
    while ('0' <= *ep && *ep <= '9') v = v * 10 + *ep++ - '0';
  }
  if (*ep == ';') ep++;
  return v;
}

void tput(int c)
{
  int i;
  static int en, state;
  static char e[BUFSIZ];
  form->redraw = 1;
  switch (state) {
  case 0: // normal text
    switch (c) {
    case '\e': state = 1; return;
    case '\r': x = 0; return;
    case '\b': if (--x < 0) x = 0; return;
    default: // XXX if c > 31 ???
      crt[i = y*80 + x] = c;
      if (reverse) { bga[i] = bold ? fg + 8 : fg; fga[i] = underline ? bg + 8 : bg; }
      else         { fga[i] = bold ? fg + 8 : fg; bga[i] = underline ? bg + 8 : bg; }
      if (++x < 80) return;
    case '\n':
    case '\f':
    case '\v':
      x = 0;
      if (++y < 25) return;
      memcpy(crt, &crt[80], 24*80);
      memcpy(fga, &fga[80], 24*80);
      memcpy(bga, &bga[80], 24*80);
      clr(24*80, 80);
      y = 24;
      return;
    case '\a': // XXX ring a bell?
    case 0:
    case 127: return;
    case '\t': if ((x = (x + 8) & -8) > 79) x = 79; return; // XXX should \n if at eol?
    }
  case 1: // \e
    switch (c) {
    case '\e': return;
    case '[': state = 2; ep = e; en = 0; return;
    case ']': state = 3; en = 0; return;
    }
    state = 0; return;
  case 2: // \e[
    if (en >= sizeof(e)) { state = 0; return; }
    switch (e[en++] = c) {
    case 'A': if ((y -= arg(1)) < 0) y = 0; break; // up
    case 'B': if ((y += arg(1)) > 24) y = 24; break; // down
    case 'C': if ((x += arg(1)) > 79) x = 79; break; // right
    case 'D': if ((x -= arg(1)) < 0) x = 0; break; // left
    case 'H': if ((y = arg(1) - 1) < 0) y = 0; else if (y > 24) y = 24; // pos row, col
    case 'G': if ((x = arg(1) - 1) < 0) x = 0; else if (x > 79) x = 79; break; // pos col
    case 'J':
      switch (arg(0)) { // clear screen
      case 0: clr(y*80 + x, 25*80 - (y*80 + x)); break; // below
      case 1: clr(0, y*80 + x); break; // above
      case 2: clr(0, 25*80); break; // all
      }
      break;
    case 'K':
      switch (arg(0)) { // clear line
      case 0: clr(y*80 + x, 80 - x); break; // right
      case 1: clr(y*80, x); break; // left
      case 2: clr(y*80, 80); break; // all
      }
      break;
    case 'm': // rendition
      for (;;) {
        switch(i = arg(-1)) {
        case -1: state = 0; return;
        case 0: reverse = underline = bold = 0; fg = def_fg; bg = def_bg; continue;
        case 1: bold = 1; continue;
        case 4: underline = 1; continue;
        case 7: reverse = 1; continue;
        case 22: bold = 0; continue;
        case 24: underline = 0; continue;
        case 27: reverse = 0; continue;
        case 38: if (arg(-1) == 5 && (uint)(i = arg(-1)) < 256) fg = i; continue;
        case 39: fg = def_fg; continue;
        case 48: if (arg(-1) == 5 && (uint)(i = arg(-1)) < 256) bg = i; continue;
        case 49: bg = def_bg; continue;
        case 30 ... 37: fg = i - 30; continue;
        case 40 ... 47: bg = i - 40; continue;
        case 90 ... 97: fg = i - 82; continue;
        case 100 ... 107: bg = i - 92; continue;
        }
      }
    case 'h':
      if (e[0] == '?') {
        ep++;
        switch (i = arg(-1)) {
        case 25: hidecursor = 0; break;
        case 9: case 1000: case 1002: trackmouse = i; break;
        }
      }
      break;
    case 'l':
      if (e[0] == '?') {
        ep++;
        switch (arg(-1)) {
        case 25: hidecursor = 1; break;
        case 9: case 1000 ... 1003: trackmouse = 0; break;
        }
      }
      break;
    case 't': // 8;row;col // set window size XXX
//      if (arg(-1) == 8) {
//      }
      break;
    case '\e': state = 1; return;
    case '0' ... '9': case ';': case '?': return;
    }
    state = 0; return;
  case 3: state = (c == '0') ? 4 : 0; return; // \e]
  case 4: state = (c == ';') ? 5 : 0; return; // \e]0
  case 5: // \e]0;
    if (c == '\a') { e[en] = 0; fl_set_form_title(form, e); state = 0; return; } // \e]0;title\a
    if (en >= sizeof(e)) { state = 0; return; }
    e[en++] = c;
  }
}

void my_rect(float x, float y, float w, float h)
{
  glBegin(GL_QUADS); glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h); glEnd();
}

void draw_term(FL_OBJECT *ob) // attempt to be efficient
{
  uint i, j, k, n, cc, c; uchar *s,*t;
  fl_rect(ob->x, ob->y, ob->w, ob->h, cc = pal[def_bg]); // XXX  clear color?
  for (i=0;i<25;i++) {
    k = ob->y + i*12;
    t = &bga[(24-i)*80];
    for (j=0;j<80;) {
      if (t[j] == def_bg) { j++; continue; }
      c = t[j];
      for (n=j+1;n<80;n++) if (t[n] != c) break;
      c = pal[c];
      if (cc != c) fl_color(cc = c);
      my_rect(ob->x + j*6, k, (n-j)*6, 12);
      j = n;
    }
  }
  glEnable(GL_TEXTURE_2D);
  glListBase(fontbase - 32);
  for (i=0;i<25;i++) {
    k = ob->y + i*12;
    s = &crt[(24-i)*80];
    t = &fga[(24-i)*80];
    for (j=0;j<80;) {
      if (!s[j]) { j++; continue; }
      c = t[j];
      for (n=j+1;n<80;n++) if (!s[n] || t[n] != c) break;
      c = pal[c];
      if (cc != c) fl_color(cc = c);
      glPushMatrix();
      glTranslatef(ob->x + j*6, k, 0.0); 
      glCallLists(n-j, GL_UNSIGNED_BYTE, s+j);
      glPopMatrix();
      j = n;
    }        
  }
  glDisable(GL_TEXTURE_2D);
  if (!hidecursor) fl_draw_box(FL_BORDER_BOX, ob->x + x*6, ob->y + (24-y)*12, 6, 12, 0, FL_RED);
}

int handle_term(FL_OBJECT *ob, int event, int x, int y)
{
  uint k; static int mx, my;
  switch(event) {
  case REDRAW:
    draw_term(ob);
    break;
  case KEY:
    if (x < 256) { write(p1[1], &x, 1); break; }
    k = '1' + (x & KEY_SHIFT ? 1 : 0) + (x & KEY_CTRL ? 4 : 0);
    if ((x &= 0x1ff) >= KEY_UP && x <= KEY_LEFT) dprintf(p1[1], "\e[1;%c%c", k, 'A' - KEY_UP + x);
    else dprintf(p1[1], "\e[%d;%c~", 1 - KEY_HOME + x, k);
    break;
  case LDOWN:  if (trackmouse)         dprintf(p1[1], "\e[M%c%c%c", 32, mx=x/6+32, my=y/12+32); break;  // XXX font metrics
  case LUP:    if (trackmouse >= 1000) dprintf(p1[1], "\e[M%c%c%c", 35, mx=x/6+32, my=y/12+32); break;
  case LMOUSE: if (trackmouse == 1002) { x=x/6+32; y=y/12+32; if (mx != x || my != y) dprintf(p1[1], "\e[M%c%c%c", 64, mx=x, my=y); }
  }
  return 0;
}

void init_pal(void)
{
  int i, r, g, b;
  for(i = 16, r = 0; r < 6; r++) for(g = 0; g < 6; g++) for(b = 0; b < 6; b++)
    pal[i++] = (r ? 0x37000000 + 0x28000000 * r : 0) + (g ? 0x00370000 + 0x00280000 * g : 0) + (b ? 0x000037ff + 0x00002800 * b : 0xff);
  for (r = 0; r < 24; r++)
    pal[i++] = 0x080808ff + 0x0a0a0a00 * r;
}

//void sigchld(int a) {
//	int stat = 0;
//	if(waitpid(pid, &stat, 0) < 0)
//		die("Waiting for pid %hd failed: %s\n",	pid, SERRNO);
//	if(WIFEXITED(stat))
//		exit(WEXITSTATUS(stat));
//	else
//		exit(-1);
//}

int main(int argc, char *argv[])
{
  int sd, e, w, x, y;
  static uchar buf[64*1024]; // XXX if this is on stack causes exception in interrupt handler!  also if I run ./sdk or large i/o task!!
  char *args[3], *title, **command;
  struct pollfd pfd[2];
//  char resize;  XXX implement resize
  FL_OBJECT *ob;
  
  def_fg = 7;
  command = 0;
  title = "vt";
  while (--argc) {
    argv++;
    if (!strcmp(*argv,"-host") && argc > 1) { ghost(*++argv); argc--; }
    else if (!strcmp(*argv,"-port") && argc > 1) { gport(atoi(*++argv)); argc--; } 
    else if (!strcmp(*argv,"-t") && argc > 1) { title = *++argv; argc--; }
    else if (!strcmp(*argv,"-e") && argc > 1) { command = ++argv; argc--; break; }
    else if (!strcmp(*argv,"-d")) debug = 1;
    else { dprintf(2,"usage: vt [-e] [-host n] [-port n] [-t title] [-e command]\n"); exit(1); }
  }

  sd = fl_initialize();

  fg = def_fg;
  bg = def_bg;
  init_pal();
  
  form = fl_bgn_form(FL_NO_BOX, 500, 310); // XXX make room for and implement scrollbars
  ob = fl_make_object(FL_BOX, FL_BOX, 0, 0, 80*6, 25*12, 0, handle_term);
  fl_add_object(form, ob);
  fl_end_form();
  
  fl_set_focus_object(form, ob);
  
  clr(0, 25*80);
  
  pipe(p1);
  pipe(p2);
  
  if (!command) {
    args[0] = "/bin/sh";
    args[1] = "-i";
    args[2] = 0;
    command = args;
  }
  if (!fork()) {
    close(sd);
    dup2(p1[0], 0);
    dup2(p2[1], 1);
    dup2(p2[1], 2);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exec(command[0], command);
    exit(0); 
  }
  close(p1[0]);
  close(p2[1]);
  
//  signal(SIGCHLD, sigchld);

  fl_show_form(form, 0, 0, title);

//  resiz = KEY_REPORT;  // XXX
  pfd[0].fd = sd;
  pfd[0].events = POLLIN;
  pfd[1].fd = p2[0];
  pfd[1].events = POLLIN;
  for (;;) {
    if ((y = poll(pfd, 2, -1)) < 0) { if (debug) printf("poll()=%d\n", y); break; }
    if (pfd[1].revents & POLLIN) {
      if ((y = read(p2[0], buf, sizeof(buf))) <= 0) { if (debug) printf("read(pipe)=%d\n", y); break; }
      for (x = 0; x < y; x++) tput(buf[x]);
    }
    if (pfd[0].revents & POLLIN) {
      e = qread(&w, &x, &y);
      if (e == HANGUP || e == WINSHUT) { if (debug) printf("qread()=%s\n", e==HANGUP ? "HANGUP" : "WINSHUT"); break; }
//      if (e == SHAPE && rawmode) write(p1[1], &resiz, 1);  // XXX no!!  send a SIGWINCH signal 
      fl_handle_forms(e, w, x, y);
    }

    fl_draw_forms(); // XXX update only every 200ms
  }
  gexit();
  return 0;
}
