// os3.c -- time slice between two user tasks and implement write syscall

#include <u.h>

enum {    // processor fault codes
  FMEM,   // bad physical address
  FTIMER, // timer interrupt
  FKEYBD, // keyboard interrupt
  FPRIV,  // privileged instruction
  FINST,  // illegal instruction
  FSYS,   // software trap
  FARITH, // arithmetic trap
  FIPAGE, // page fault on opcode fetch
  FWPAGE, // page fault on write
  FRPAGE, // page fault on read
  USER=16 // user mode exception 
};

char task0_stack[1000];
char task0_kstack[1000];

char task1_stack[1000];
char task1_kstack[1000];

int *task0_sp;
int *task1_sp;

int current;

out(port, val)  { asm(LL,8); asm(LBL,16); asm(BOUT); }
ivec(void *isr) { asm(LL,8); asm(IVEC); }
stmr(int val)   { asm(LL,8); asm(TIME); }
halt(value)     { asm(LL,8); asm(HALT); }

sys_write(fd, char *p, n) { int i; for (i=0; i<n; i++) out(fd, p[i]); return i; }

write() { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_write); }

task0()
{
  while(current < 10)
    write(1, "00", 2);

  write(1,"task0 exit\n", 11);
  halt(0);
}

task1()
{
  while(current < 10)
    write(1, "11", 2);

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

trap(int *sp, int c, int b, int a, int fc, unsigned *pc)
{
  switch (fc) {
  case FSYS + USER: // syscall
    switch (pc[-1] >> 8) {
    case S_write: a = sys_write(a, b, c); break;
    default: sys_write(1, "panic! unknown syscall\n", 23); asm(HALT);
    }
    break;
    
  case FTIMER:  // timer
  case FTIMER + USER:
    out(1,'x');
    if (++current & 1)
      swtch(&task0_sp, task1_sp);
    else
      swtch(&task1_sp, task0_sp);
    break;
    
  default:
    default: sys_write(1, "panic! unknown interrupt\n", 25); asm(HALT);  
  }
}

alltraps()
{
  asm(PSHA);
  asm(PSHB);
  asm(PSHC);
  asm(LUSP); asm(PSHA);
  trap();                // registers passed by reference/magic
  asm(POPA); asm(SUSP);
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

trapret()
{
  asm(POPA); asm(SUSP);
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

main()
{
  int *kstack;
  
  stmr(5000);
  ivec(alltraps);

  task0_sp = &task0_kstack[1000];
  task0_sp -= 2; *task0_sp = &task0;
  task0_sp -= 2; *task0_sp = USER; // fault code
  task0_sp -= 2; *task0_sp = 0; // a
  task0_sp -= 2; *task0_sp = 0; // b
  task0_sp -= 2; *task0_sp = 0; // c
  task0_sp -= 2; *task0_sp = &task0_stack[1000];
  task0_sp -= 2; *task0_sp = &trapret;  
  
  task1_sp = &task1_kstack[1000];
  task1_sp -= 2; *task1_sp = &task1;
  task1_sp -= 2; *task1_sp = USER; // fault code
  task1_sp -= 2; *task1_sp = 0; // a
  task1_sp -= 2; *task1_sp = 0; // b
  task1_sp -= 2; *task1_sp = 0; // c
  task1_sp -= 2; *task1_sp = &task1_stack[1000];
  task1_sp -= 2; *task1_sp = &trapret;

  kstack = task0_sp;
  
  asm(LL, 4); // a = kstack
  asm(SSP);   // sp = a
  asm(LEV);
}
