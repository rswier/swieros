// edit -- text editor
//
// Usage:  edit file

// Based on the public domain ea.c Anthony Howe.

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <curses.h>

enum {
  BUF   = 1024*1024,
  LINES = 24,
  COLS  = 80,
};

int col, ind, x, page, pagex = -1, gap, egap, ebuf, line = 1;
int lines;

char buf[BUF], *filename;

xmemmove(char *d, char *s, uint n) { if (d > s && s + n > d) { while (n--) d[n] = s[n]; } else memcpy(d, s, n); }

movegap()
{
  if (x < gap) xmemmove(buf + egap + x - gap, buf + x, gap - x);
  else memcpy(buf + gap, buf + egap, x - gap);
  egap += x - gap;
  gap = x;
}

int prev(int i)
{
  while (i > gap) if (buf[--i + egap - gap] == '\n') return i + 1;
  while (i > 0) if (buf[--i] == '\n') return i + 1;
  return 0;
}

next()
{
  while (x < gap) if (buf[x++] == '\n') return;
  while (x < ebuf + gap - egap) if (buf[x++ + egap - gap] == '\n') return;
}

up()
{
  page = prev(page - 1);
  if (line > 1) line--;
}

down()
{
  while (page < gap) if (buf[page++] == '\n') { line++; return; }
  while (page < ebuf + gap - egap) if (buf[page++ + egap - gap] == '\n') { line++; return; }
}

adjust()
{
  int i; char c;
  i = 0;
  while (i < col && x < ebuf + gap - egap && (c = buf[x < gap ? x : x + egap - gap]) != '\n') {
    i += (c == '\t') ? 8-(i&7) : 1; // XXX
    x++;
  }
}

display()
{
  int i, j, p, row; char c;
  static int indx;

  while (page > x) up();
  i = j = 0;
  for (p = page; ; p++) {
    if (x == p) { row = i; col = j; }
    if ((p >= x && i >= LINES-1) || p == ebuf + gap - egap) break;
    c = buf[p < gap ? p : p + egap - gap];
    if (c != '\r' && c != '\n') {
      j += (c == '\t') ? 8-(j&7) : 1; // XXX tab issues
    }
    if (c == '\n') { i++; j = 0; }
  }

  while (i < LINES-1 && page) { i++; row++; up(); }
  while (row >= LINES) { row--; down(); }

  if (col < ind) ind = 0;
  while (col - ind >= COLS) ind += 8;

  if (pagex != page || indx != ind) {
    pagex = page; indx = ind;
    clear();
    mvprintw(i=0,j=0,"[%04d] ", line);
    for (p = page; ; p++) {
      if (i == LINES || p == ebuf + gap - egap) break;
      c = buf[p < gap ? p : p + egap - gap];
      if (c != '\r' && c != '\n' && j < ind + COLS) {
        if (j >= ind) addch(c);
        j += (c == '\t') ? 8-(j&7) : 1; // XXX insert spaces cause of word wrap issues
      }
      if (c == '\n') { if (++i < LINES) mvprintw(i, j=0, "[%04d] ", line + i); }
    }
  }

  move(row, col - ind + 7);
  refresh();
}

save()
{
  int i;
  if ((i = open(filename, O_WRONLY | O_CREAT | O_TRUNC)) < 0) return; 
  if (gap) write(i, buf, gap);
  if (egap < ebuf) write(i, buf + egap, ebuf - egap);
  close(i);
}

int main(int argc, char **argv)
{
  int i;
  if (argc < 2) { dprintf(2,"usage:  edit file\n"); exit(-1); }
  initscr();
  keypad(1);
  ebuf = egap = BUF;
  if ((i = open(filename = argv[1], O_RDONLY)) >= 0) {
    if ((gap = read(i, buf, BUF)) < 0) gap = 0;
    close(i);
  }
  
//  for (gap = 0; (n = read(i, buf + gap, BUF - gap)) > 0; gap += n) ;
//  for (p = buf; p < buf + gap && (p = memchr(p, '\n', gap - p)); lines++, p++) ;
  
  for (;;) {
    display();
    switch (i = getch())
    {
    case KEY_UP:    x = prev(prev(x) - 1); adjust(); break;
    case KEY_DOWN:  next(); adjust(); break;  
    case KEY_LEFT:  if (x > 0) x--; break;
    case KEY_RIGHT: if (x < ebuf + gap - egap) x++; break;
    case KEY_PPAGE: x = prev(x); for (i=0; i<LINES-2; i++) { up(); x = prev(x - 1); } adjust(); break;
    case KEY_NPAGE: for (i=0; i<LINES-2; i++) { down(); next(); } page = prev(page); adjust(); break;
    case KEY_HOME:  x = page = 0; line = 1; break;
    case KEY_END:   x = ebuf + gap - egap; break;
    case KEY_DC:    if (x == ebuf + gap - egap) break; if (x != gap) movegap(); if (buf[egap] == '\n') lines--; egap++; pagex = -1; break; // XXX del \r\n
    case KEY_F1:    save(); break;
    case '\e':      goto done;
    case '\b':      if (x == 0) break; if (x != gap) movegap(); gap--; x = gap; if (buf[gap] == '\n') lines--; pagex = -1; break;
    case '\r':      if (gap == egap) break; if (x != gap) movegap(); buf[gap++] = '\n'; lines++; x = gap; pagex = -1; break;
    default:        if (gap == egap) break; if (x != gap) movegap(); buf[gap++] = i; x = gap; pagex = -1; break;
    }
  }
done:
  endwin();
  return 0;
}
