// bin2c -- convert binary file to c source
//
// Usage:  bin2c file name
//
// Description:
//   bin2c converts a binary file to c source code.  The first argument
//   is the raw file name.  the second argument is the name of the array
//   generated.

#include <u.h>
#include <libc.h>

int main(int argc, char *argv[])
{
  char buf[1024];
  int f, i, n;
  if (argc != 3) { dprintf(2, "Usage: bin2c file name\n"); return -1; }
  if ((f = open(argv[1], O_RDONLY)) < 0) { dprintf(2, "Cannot open input file.\n"); return -1; }
  
  printf("unsigned char %s[] = {", argv[2]);
  while ((n = read(f, buf, sizeof(buf))) > 0) {
    for (i=0;i<n;i++) {
      if (!(i&15)) printf("\n  ");
      printf("%u,",buf[i]);
    }
  }
  printf("\n};\n");
  return 0;
}
