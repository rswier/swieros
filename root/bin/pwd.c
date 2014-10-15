// pwd -- print working directory
//
// Usage:  pwd
//
// Description:
//   pwd prints the current working directory.

#include <u.h>
#include <libc.h>
#include <dir.h>

int main(int argc, char *argv[])
{
  char buf[512];
  if (argc != 1) { dprintf(2, "usage: pwd\n"); return -1; }
  if (!getcwd(buf, sizeof(buf))) return -1;
  printf("%s\n",buf);
  return 0;
}
