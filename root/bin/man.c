// man -- command manual
//
// Usage:  man command
//
// Description:
//   man prints the user manual for commands found in /bin.

#include <u.h>
#include <libc.h>

int getc(int f) // XXX rewrite
{
  // only works correctly if f never varies!
  static uchar buf[1024], *p; 
  static int n = 0;  
  if (n == 0 && (n = read(f, (p = buf), sizeof(buf))) <= 0) { n = 0; return -1; }
  --n;
  return *p++;
}

char *fgets(char *s, int n, int f) // XXX rewrite
{
  char c, *cs;
  cs = s;
  while (--n > 0 && (c = getc(f)) >= 0) if ((*cs++ = c) == '\n') break;
  if (c < 0 && cs == s) return 0;
  *cs = '\0';
  return s;
}

puts(char *s)
{
  write(1, s, strlen(s));
}

int main(int argc, char *argv[])
{
  int f, n, i; char s[256]; char buf[1024];
  if (argc != 2) { dprintf(2,"usage: man command\n"); return -1; }
  sprintf(s, "/bin/%s.c", argv[1]);
  if ((f = open(s, O_RDONLY)) < 0) { dprintf(2,"error: no man page for %s\n",s); return -1; }
  while (fgets(buf, sizeof(buf), f) && buf[0] == '/' && buf[1] == '/') puts(buf + 2 + (buf[2] == ' '));
  close(f);
  return 0;
}