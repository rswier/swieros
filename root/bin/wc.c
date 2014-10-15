// wc.c XXX revamp

#include <u.h>
#include <libc.h>

void wc(int fd, char *name)
{
  int i, n, l, w, c, inword; char buf[512];

  l = w = c = 0;
  inword = 0;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i=0; i<n; i++) {
      c++;
      if (buf[i] == '\n')
        l++;
      if (strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if (!inword) {
        w++;
        inword = 1;
      }
    }
  }
  if (n < 0) {
    dprintf(2,"wc: read error\n");
    exit(0);
  }
  printf("%d %d %d %s\n", l, w, c, name);
}

int main(int argc, char *argv[])
{
  int fd, i;

  if (argc <= 1) { wc(0, ""); return 0; }

  for (i = 1; i < argc; i++) {
    if( (fd = open(argv[i], 0)) < 0) {
      dprintf(2,"wc: cannot open %s\n", argv[i]);
      return -1;
    }
    wc(fd, argv[i]);
    close(fd);
  }
  return 0;
}
