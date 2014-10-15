// cp -- copy file
//
// Usage:  cp file1 file2
//
// Description:
//   cp copies the file1 into file2

#include <u.h>
#include <libc.h>

int main(int argc, char *argv[])
{
  char buf[512];
  struct stat sin, sout;
  int fin, fout, n;

  if (argc != 3) { dprintf(2, "Usage: cp file1 file2\n"); return -1; }
  if ((fin = open(argv[1], O_RDONLY)) < 0) { dprintf(2, "Cannot open input file.\n"); return -1; }
  if (fstat(fin, &sin)) { dprintf(2, "Cannot stat input file.\n"); return -1; }
  
  if ((fout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC)) < 0) { dprintf(2, "Cannot create output file.\n"); return -1; }
  if (!fstat(fout, &sout) && sin.st_ino == sout.st_ino) { dprintf(2, "Copying file to itself.\n"); return -1; }
  while ((n = read(fin, buf, sizeof(buf))) > 0) {
    if (write(fout, buf, n) != n) { dprintf(2, "Write error.\n"); return -1; }
  }
  if (n < 0) { dprintf(2, "Read error\n"); return -1; }
  return 0;
}
