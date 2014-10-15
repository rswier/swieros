#include <u.h>

write(int fd, void *buf, int len)
{
  asm(LL,8);         // load register a with fd
  asm(LBL,16);       // load register b with buf
  asm(LCL,24);       // load register c with len
  asm(TRAP,S_write); // trap to OS write syscall.  result will be in register a
}

main()
{
  write(1, "Hello world.\n", 13);
  return 0;
}
