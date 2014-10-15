// os1.c -- time slice between two kernel tasks

#include <u.h>

char task1_stack[200];
int *task0_sp;
int *task1_sp;

int current;

out(port, val)  { asm(LL,8); asm(LBL,16); asm(BOUT); }
ivec(void *isr) { asm(LL,8); asm(IVEC); }
stmr(int val)   { asm(LL,8); asm(TIME); }
halt(value)     { asm(LL,8); asm(HALT); }

write(fd, char *p, n) { while (n--) out(fd, *p++); }

task0()
{
  while(current < 10) {
    write(1, "0", 1);
  }
  write(1,"task0 exit\n", 11);
  halt(0);
}

task1()
{
  while(current < 10) {
    write(1, "1", 1);
  }
  write(1,"task1 exit\n", 11);
  halt(0);
}

swtch(int *old, int new) // switch stacks
{
  asm(LEA, 0); // a = sp
  asm(LBL, 8); // b = old
  asm(SX, 0);  // *b = a
  asm(LL, 16); // a = new
  asm(SSP);    // sp = a
}

trap()
{
  if (++current & 1)
    swtch(&task0_sp, task1_sp);
  else
    swtch(&task1_sp, task0_sp);
}

alltraps()
{
  asm(PSHA);
  asm(PSHB);
  asm(PSHC);

  trap();
    
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

trapret()
{
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

main()
{
  current = 0;

  stmr(5000);
  ivec(alltraps);
  
  task1_sp = &task1_stack;
  task1_sp += 50;
  
  
  task1_sp -= 2; *task1_sp = &task1;
  task1_sp -= 2; *task1_sp = 0; // fc
  task1_sp -= 2; *task1_sp = 0; // a  
  task1_sp -= 2; *task1_sp = 0; // b  
  task1_sp -= 2; *task1_sp = 0; // c  
  task1_sp -= 2; *task1_sp = &trapret;  
  
  asm(STI);
  
  task0();
}
