#include <u.h>

out(port, val)
{
  asm(LL,8);   // load register a with port
  asm(LBL,16); // load register b with val
  asm(BOUT);   // output byte to console
}

write(int f, char *s, int n)
{
  while (n--)
    out(f, *s++);
}  
  
main()
{
  write(1, "Hello world.\n", 13);
  asm(HALT);
}
