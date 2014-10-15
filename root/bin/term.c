// term -- terminal client program
//
// Usage:  term [-p port]
//
// Description:
//   term is a simple console to socket application.  This allows remote access to the system
//   by connecting to the shd shell daemon.

#include <u.h>
#include <libc.h>
#include <net.h>

int main(int argc, char *argv[])
{
  int sd, n, port = 5002;
  struct sockaddr_in addr;
  static char line[1024];
  struct pollfd pfd[2];

  while (--argc && (*++argv)[0] == '-') {
    switch ((*argv)[1]) {
    case 'p': port = atoi(*++argv); break;
    default: dprintf(2,"unknown argument\n"); return -1;
    }
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { dprintf(2,"socket() failed\n"); return -1; }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(0x7f000001); // XXX 

  if (connect(sd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) { dprintf(2,"connect() failed\n"); return -1; }

  pfd[0].fd = 0;
  pfd[0].events = POLLIN;
  pfd[1].fd = sd;
  pfd[1].events = POLLIN;
  for (;;) {
    if (poll(pfd, 2, -1) < 0) break;
    if (pfd[0].revents & POLLIN) {
      if ((n = read(0, line, sizeof(line))) > 0) write(sd, line, n); else break;
    }
    if (pfd[1].revents & POLLIN) {
      if ((n = read(sd, line, sizeof(line))) > 0) write(1, line, n); else break;
    }
  }
  return 0;
}
