// sdk -- sudoku solver
//
// Usage: sdk [solution_number]
//
// Description:
//   Solves the game of sudoku.  A parameter may be passed to indicate the solution number you
//   want to solve.  A solution number of zero will generate all solutions.
//   Press 'q' or ESC at any time to quit.

#include <u.h>
#include <libc.h>
#include <curses.h>

int board[81] = // set to what you want to solve
{
  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,

  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,

  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,
  0,0,0,  0,0,0,  0,0,0,
};

int known[81];

void print_pos(int i)
{
  int r = i/9, c = i%9;
  mvprintw(r + r/3 + 2, (c + c/3)*2 + 2, "%d", board[i]);
  refresh();
}

int validate(int p)
{
  int i, r = p/9, c = p%9, x = board[p];

  // validate row
  c = (c + 1) % 9;
  for (i=0;i<8;i++) {
    if (board[r*9+c] == x) return 0;
    c = (c + 1) % 9;
  }

  // validate column
  r = (r + 1) % 9;
  for (i=0;i<8;i++) {
    if (board[r*9+c] == x) return 0;
    r = (r + 1) % 9;
  }

  // validate square
  if (!(++c % 3)) { c -= 3; if (!(++r % 3)) r -= 3; }
  for (i=0;i<8;i++) {
    if (board[r*9+c] == x) return 0;    
    if (!(++c % 3)) { c -= 3; if (!(++r % 3)) r -= 3; }
  }

  return 1;
}

int main(int argc, char *argv[])
{
  int i, j, s, c;

  s = (argc == 2) ? atoi(argv[1]) : 1;
  initscr();
  curs_set(0);
  nodelay(1);

  for (i=0;i<81;i++) {
    known[i] = board[i] ? 1 : 0;
    print_pos(i);
  }

  i = j = 0;
  mvprintw(0,0,"Sudoku solver");
  for (;;) {
    if ((c = getch()) != ERR && (c == '\e' || c == 'q')) goto done;
    if (i == 81) {
      mvprintw(0,0,"Sudoku solver %d ",++j);
      if (j == s) goto done; 
      i--;
      while (known[i]) { i--; if (i < 0) goto done; }
    } else {
      if (known[i]) { i++; continue; }
      board[i]++;
      print_pos(i);
      if (validate(i)) { i++; continue; }
    }
    while (board[i] == 9) {
      board[i] = 0;
      print_pos(i);
      if (--i < 0) goto done;
      while (known[i]) { i--; if (i < 0) goto done; }
    }
  }
done:
  mvprintw(14, 0, "Done!\n");
  endwin();
  return 0;
}
