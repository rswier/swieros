// prseg - print segment addresses

#include <u.h>

int data = 1;
int bss;

void *sbrk();

int main()
{
  int stack;
  printf("code  = 0x%08x\n", &main);
  printf("data  = 0x%08x\n", &data);
  printf("bss   = 0x%08x\n", &bss);
  printf("stack = 0x%08x\n", &stack);
  printf("brk   = 0x%08x\n", sbrk(0));
  return 0;
}

#include <libc.h>
