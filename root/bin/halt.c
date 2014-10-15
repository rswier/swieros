// halt.c

#include <u.h>

main()
{
  asm(LI,0);
  asm(HALT); // XXX supervisor mode, replace with shutdown syscall
}
