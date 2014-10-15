// init.c - first process to run

#include <u.h>
#include <libc.h>

main()
{
  char *argv[3];
  int pid, wpid;

  argv[0] = "/bin/sh";
  argv[1] = "-i";
  argv[2] = 0;

  if (open("/dev/console", O_RDWR) < 0) {
    mknod("/dev/console", S_IFCHR, (1 << 8) | 1);
    open("/dev/console", O_RDWR);
  }
  dup2(0,1);
  dup2(0,2);
  
  chdir("/usr");
  for (;;) {
    if ((pid = fork()) < 0) { printf("init: fork failed\n"); return -1; }
    if (!pid) {
      exec(argv[0], argv);
      printf("init: exec sh failed\n");
      return -1;
    }
    while ((wpid = wait()) >= 0 && wpid != pid)
      printf("zombie!\n");
  }
}
