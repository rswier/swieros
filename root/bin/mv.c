// mv -- move files
//
// Usage:  mv file1 file2
//
// Description:
//   mv moves/renames a file from one place to another.

#include <u.h>
#include <libc.h>

int mv(char *from, char *to)
{
	if (link(from, to)) { dprintf(2,"mv: cannot link to %s\n", to); return -1; }
	if (unlink(from)) { dprintf(2,"mv: cannot unlink %s\n", from); return -1; }
	return 0;
}

int main(int argc, char *argv[])
{
  if (argc != 3) { dprintf(2, "Usage: mv file1 file2\n"); return -1; }
  return mv(argv[1], argv[2]);  
}
