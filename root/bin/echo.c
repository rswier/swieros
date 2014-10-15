// echo -- echo arguments
//
// Usage:  echo arg ...
//
// Description:
//   echo writes its arguments in order to stdout.

#include <u.h>
#include <libc.h>

int main(int argc, char *argv[])
{
  int i;
  for(i = 1; i < argc; i++)
    printf("%s%c", argv[i], (i+1 < argc) ? ' ' : '\n');
  return 0;
}
