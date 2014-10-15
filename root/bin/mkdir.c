// mkdir -- make a directory
//
// Usage:  mkdir dirname ...
//
// Description:
//   mkdir creates the specified directories.

#include <u.h>
#include <libc.h>

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2) { dprintf(2, "Usage: mkdir dirname ...\n"); return -1; }

  for (i = 1; i < argc; i++) {
    if (mkdir(argv[i]) < 0) { dprintf(2, "mkdir: %s failed to create\n", argv[i]); return -1; }
  }

  return 0;
}
