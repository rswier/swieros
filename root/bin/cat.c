// cat -- concatenate and print
//
// Usage:  cat file ...
//
// Description:
//   cat reads each file in sequence and writes it to stdout.  If no input is
//   given, cat reads from stdin.

#include <u.h>
#include <libc.h>

void cat(int fd)
{
  char buf[4096]; int n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) { dprintf(2,"cat: write error\n"); exit(-1); }
  }
  if (n < 0) { dprintf(2,"cat: read error\n"); exit(-1); }
}

int main(int argc, char *argv[])
{
  int i, fd;

  if (argc <= 1) { cat(0); return 0; }
  
  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], O_RDONLY)) < 0) { dprintf(2,"cat: cannot open %s\n", argv[i]); return -1; }
    cat(fd);
    close(fd);
  }
  return 0;
}
