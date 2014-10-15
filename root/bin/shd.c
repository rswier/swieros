// shd -- shell daemon
//
// Usage:  shd [port]
//
// Description:
//   shd launches a shell daemon allowing remote access to the system over a TCP socket.

#include <u.h>
#include <libc.h>
#include <net.h>

int main(int argc, char *argv[])
{
  int ld, sd, r;
  static struct sockaddr_in addr; 
  char *av[3];
 
  if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) { dprintf(2,"socket() failed\n"); return -1; }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((argc == 2) ? atoi(argv[1]) : 5002);
  if (bind(ld, (struct sockaddr *) &addr, sizeof(addr))) { dprintf(2,"bind() failed\n"); return -1; }
  if (listen(ld, 1) < 0) { dprintf(2,"listen() failed\n"); return -1; }
  
  for (;;) {
    if ((sd = accept(ld, 0, 0)) < 0) { dprintf(2,"accept() failed\n"); return -1; }
    if ((r = fork()) < 0) { dprintf(2,"fork() failed\n"); return -1; }
    if (!r) {
      dup2(sd,0);
      dup2(sd,1);
      dup2(sd,2);
      close(sd);
      close(ld);
      av[0] = "/bin/sh";
      av[1] = "-i";
      av[2] = 0;
      exec(av[0], av);
      return 0;
    }
    close(sd);
  }
}
