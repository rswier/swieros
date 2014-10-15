// rm -- remove files
//
// Usage:  rm file ...
//
// Description:
//   rm removes the entries for one or more files from a directory.

#include <u.h>
#include <libc.h>

int main(int argc, char *argv[])
{
  int i;

  if (argc < 2) { dprintf(2, "Usage: rm file ...\n"); return -1; }

  for(i = 1; i < argc; i++) {
    if(unlink(argv[i])) {
      dprintf(2, "rm: failed to delete %s\n", argv[i]);
      return -1;
    }
  }
  return 0;
}
