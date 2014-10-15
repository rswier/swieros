// fsd -- file server daemon

#include <u.h>
#include <libc.h>
#include <net.h>

enum { M_OPEN, M_CLOSE, M_READ, M_WRITE, M_SEEK, M_FSTAT, M_SYNC };

// M_LINK, M_UNLINK, M_MKDIR

int debug;
int verbose;
int sync; // XXX

int sd = -1;
int fd = -1;
char *s;

void fatal(char *s)
{
  printf("fatal error: %s\n",s);
  exit(-1);
}

void tx(void *p, uint n)
{
  int r;
  while (n) {
    if ((r = write(sd, p, n)) <= 0) fatal("tx()");
    n -= r;
    p += r; 
  }
}

void rx(void *p, int n)
{
  int r;
  while (n) {
    if ((r = read(sd, p, n)) <= 0) { if (verbose) printf("Connection closed\n"); exit(0); }
    n -= r;
    p += r; 
  }
}

ssize(int n)
{
  static int sn = 0;
  n = (n + 4095) & -4096;
  if (n > sn) {
    if (!s) s = sbrk(n); else sbrk(n - sn);
    sn = n;
//    if (s) free(s);
//    s = malloc(n);
//    if (!s) fatal("ssize() malloc");
  }
}

void rxmsgs(void)
{
  int h[3], n, r; struct stat hs;
  
  for (;;) {
    rx(&n,4);
    switch (n) {
    case M_OPEN:
      if (fd >= 0) fatal("open");
      rx(h,12); ssize(h[2]);
      rx(s,h[2]);
//      sync = (h[0] & O_SYNC);
      sync = 0;
//      h[1] = S_IRWXU; // XXX
//      fd = open(s, h[0] | O_BINARY, h[1]); // XXX third arg?
      fd = open(s, h[0]); // XXX
      if (debug) printf("%d = open(%s, %d, %d)\n",fd,s,h[0],h[1]);
      r = (fd >= 0) ? 0 : -1;
      tx(&r,4);
      break;
    case M_CLOSE:
      if (fd < 0) fatal("close");
      close(fd);
      if (debug) printf("close(%d)\n",fd);
      fd = -1;
      break;
    case M_READ:
      if (fd >= 0) {
        rx(&n,4); ssize(n);
        r = read(fd, s, n);
        if (debug) printf("%d = read(%d, s, %d)\n", r, fd, n);
        tx(&r,4);
        if ((uint)r <= n) tx(s, r);
      } else fatal("read");
      break;
    case M_WRITE:
      if (fd < 0) fatal("write");
      rx(&n,4); ssize(n);
      if (n > 0) rx(s, n);
      r = write(fd, s, n);
      if (debug) printf("%d = write(%d, s, %d)\n", r, fd, n);
      if (sync) tx(&r,4);
      break;
    case M_SEEK:
      if (fd < 0) fatal("lseek");
      rx(h,8);
      r = lseek(fd, h[0], h[1]);
      if (debug) printf("%d = lseek(%d, %d, %d)\n", r, fd, h[0], h[1]);
      tx(&r,4);
      break;
    case M_FSTAT:
      if (fd >= 0) {
        r = fstat(fd, &hs);
        if (debug) printf("%d = fstat(%d, sp)\n", r, fd);
        tx(&r,4);
        if (r == 0) {
          tx(&hs, sizeof(hs));
        }
      } else fatal("fstat");
      break;
    case M_SYNC:
      if (fd < 0) fatal("fsync");
//      r = fsync(fd);  // XXX does win32 have this?
      if (debug) printf("%d = fsync(%d)\n", r, fd);
      tx(&r,4);
      break;
    default:
      fatal("bad message");
    }
  }
}

int main(int argc, char *argv[])
{
  int i, ld;
  struct sockaddr_in addr;
  int port = 5003;

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-v")) verbose = 1; else
    if (!strcmp(argv[i],"-d")) debug = verbose = 1; else
    if (!strcmp(argv[i],"-p") && i+1 < argc) port = atoi(argv[++i]); else
    fatal("usage: fsrv [-v] [-d] [-p port]");
  }
  
  if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) fatal("socket()");
//    i = 1; setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(ld, (struct sockaddr *) &addr, sizeof(addr)) < 0) fatal("bind()");
  if (listen(ld, 1) < 0) fatal("listen()");
  if (verbose) printf("Listening on port %d...\n", port) ;

  for (;;) {
    if ((sd = accept(ld, 0, 0)) < 0) fatal("accept()");
    if (verbose) printf("Connection accepted\n");
//      i = 1; setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
    if ((i = fork()) < 0) fatal("fork()");
    if (!i) {
      close(0);
      close(ld);
      rxmsgs();
    }
    close(sd);
  }
}
