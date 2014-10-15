// os0.c -- simple timer isr test

#include <u.h>

int current;

out(port, val)  { asm(LL,8); asm(LBL,16); asm(BOUT); }
ivec(void *isr) { asm(LL,8); asm(IVEC); }
stmr(int val)   { asm(LL,8); asm(TIME); }
halt(val)       { asm(LL,8); asm(HALT); }

alltraps()
{
  asm(PSHA);
  asm(PSHB);

  current++;

  asm(POPB);
  asm(POPA);
  asm(RTI);
}

main()
{
  current = 0;

  stmr(1000);
  ivec(alltraps);
  
  asm(STI);
  
  while (current < 10) {
    if (current & 1) out(1, '1'); else out(1, '0');
  }

  halt(0);
}
