// curses.h

enum { ERR = -1, OK, BUTTON_PRESSED, BUTTON_RELEASED, BUTTON_CLICKED = 2, REPORT_MOUSE_POSITION = 4 };
enum { A_NORMAL, A_BOLD, A_UNDERLINE, A_REVERSE = 4 };
enum { COLOR_BLACK, COLOR_BRICK, COLOR_GREEN, COLOR_GOLD,   COLOR_NAVY, COLOR_PURPLE,  COLOR_TURQUOISE, COLOR_LIGHT,
       COLOR_DARK,  COLOR_RED,   COLOR_LEAF,  COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN,      COLOR_WHITE };
enum { KEY_HOME = 0x101, KEY_IC, KEY_DC, KEY_END, KEY_PPAGE, KEY_NPAGE, KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
       KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_MOUSE, KEY_F6, KEY_F7, KEY_F8, KEY_SHIFT = 0x200, KEY_CTRL = 0x400 };
uchar _c_buf[4096]; // XXX  uchar _c_buf[BUFSIZ];  // XXX study this.. pipe issues?
int _c_n, _c_nodelay, _c_keypad;
typedef struct { int x, y, bstate; } MEVENT;

refresh() { if (_c_n) { write(1, _c_buf, _c_n); _c_n = 0; } }
addch(int c) { _c_buf[_c_n] = c; if (++_c_n == sizeof(_c_buf)) { write(1, _c_buf, _c_n); _c_n = 0; } }
addstr(char *s) { int n; if (_c_n + (n = strlen(s)) >= sizeof(_c_buf)) { refresh(); if (n >= sizeof(_c_buf)) { write(1, s, n); return; } } memcpy(_c_buf + _c_n, s, n); _c_n += n; }
nodelay(int on) { _c_nodelay = on; }
keypad(int on) { _c_keypad = on; }
kbhit() { struct pollfd pfd; pfd.fd = 0; pfd.events = POLLIN; return poll(&pfd, 1, 0); }
getchar() { uchar c; return (read(0, &c, 1) <= 0) ? -1 : c; }
int getch()
{
  int c, p, m;
  if (_c_nodelay && !kbhit()) return ERR;
  if ((c = getchar()) != '\e' || !_c_keypad || (!kbhit() && (sleep(2) || !kbhit()))) return c; // XXX adjust sleep?
  if ((c = getchar()) == 'O') {
    switch (c = getchar()) {
    case 'A' ... 'D': return KEY_UP - 'A' + c;
    case 'F': return KEY_END;
    case 'H': return KEY_HOME;
    case 'P' ... 'S': return KEY_F1 - 'P' + c;
    }
  } else if (c == '[') {
    p = m = 1;
    switch (c = getchar()) {
    case 'M': return KEY_MOUSE;
    case '[': return ((c = getchar()) >= 'A' && c <= 'E') ? KEY_F1 - 'A' + c : c;
    case '0' ... '9': p = c-'0'; while ((c = getchar()) >= '0' && c <= '9') p = p*10 + c-'0';
    }
    if (c == ';') { if ((c = getchar()) >= '0' && c <= '9') { m = c-'0'; while ((c = getchar()) >= '0' && c <= '9') m = m*10 + c-'0'; } }
    while (c == ';') { while ((c = getchar()) >= '0' && c <= '9'); }
    m--; m = (m & 1 ? KEY_SHIFT : 0) + (m & 4 ? KEY_CTRL : 0);
    switch (c) {
    case '~': return KEY_HOME - 1 + p + m;
    case 'A' ... 'D': return KEY_UP - 'A' + c + m;
    case 'F': return KEY_END + m;
    case 'H': return KEY_HOME + m;
    case 'Z': return '\t' + m;
    }
  }
  return c;
}
printw(char *f, ...) { char s[BUFSIZ]; va_list v; va_start(v, f); vsprintf(s, f, v); addstr(s); }
move(int r, int c) { printw("\e[%d;%dH", r+1, c+1); }
clear() { addstr("\e[H\e[J"); }
clrtobot() { addstr("\e[J"); }
clrtoeol() { addstr("\e[K"); }
mvaddstr(int r, int c, char *s) { move(r, c); addstr(s); }
mvaddch(int r, int c, char s) { move(r, c); addch(s); }
mvprintw(int r, int c, char *f, ...) { char s[BUFSIZ]; va_list v; va_start(v, f); vsprintf(s, f, v); move(r, c); addstr(s); }
curs_set(int on) { addstr(on ? "\e[?25h" : "\e[?25l"); }
resizeterm(int r, int c) { printw("\e[8;%d;%dt", r, c); refresh(); }
mouse_set(int e) { addstr(e & REPORT_MOUSE_POSITION ? "\e[?1002h" : (e & BUTTON_RELEASED ? "\e[?1000h" : (e & BUTTON_PRESSED ? "\e[?9h" : "\e[?9l"))); refresh(); }
getmouse(MEVENT *e)
{
  int c;
  e->bstate = (c=getchar())==3 ? BUTTON_PRESSED : (c==64 ? BUTTON_PRESSED | REPORT_MOUSE_POSITION : (c==35 ? BUTTON_RELEASED : 0));
  e->x = getchar() - 32;
  e->y = getchar() - 32;
}
charattr(int a) { printw("\e[%dm",a); }
fgcolor(int a) { if (a < 16) charattr(a < 8 ? a+30 : a+82); else printw("\e[38;5;%dm", a); }
bgcolor(int a) { if (a < 16) charattr(a < 8 ? a+40 : a+92); else printw("\e[48;5;%dm", a); }
attrset(int a) { if (!a) addstr("\e[m"); else printw("\e[%dm\e[%dm\e[%dm", (a&A_BOLD)?1:22, (a&A_UNDERLINE)?4:24, (a&A_REVERSE)?7:27); }
initscr() { clear(); refresh(); }
endwin() { curs_set(1); mouse_set(0); charattr(0); refresh(); }
