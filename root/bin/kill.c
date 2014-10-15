// kill -- kill a process
//
// Usage:  kill pid ...
//
// Description:
//   kill kills the specified processes.

#include <u.h>
#include <libc.h>

int main(int argc, char **argv)
{
  int i;

//  printf(1, "argc %d\n",argc);
//  for (i=0;i<argc;i++) printf(1, "argv[%d]=\"%s\" ",i,argv[i]);
//  printf(1, "\n");

  if (argc <= 1) { dprintf(2, "usage: kill pid ...\n"); return -1; }
  for (i=1; i<argc; i++) kill(atoi(argv[i])); // XXX need to add signal
  return 0;
}
