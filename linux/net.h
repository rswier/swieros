// linux/net.h
// Allows a few specific programs to compile and run under linux.

#include <netinet/in.h>
#include <sys/select.h>

int xsocket(int family, int ty, int protocol)
{
  int i, d;
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if ((d = socket(family, ty, protocol)) < 0) return d;
      xft[i] = xSOCKET; xfd[i] = d; //printf("socket()%d=%d\n",d,i);
      return i;
    }
  }
  return -1;  
}

// XXX use linux poll?
struct pollfd { int fd; short events, revents; };
enum { POLLIN = 1, POLLOUT = 2, POLLNVAL = 4 };  
int poll(struct pollfd *pfd, uint n, int msec)
{
  struct pollfd *p, *pn = &pfd[n];
  int f, r;
  fd_set hr, hw, *phr, *phw; struct timeval t; 
    
  phr = phw = 0;
  FD_ZERO(&hr); FD_ZERO(&hw);
  for (p = pfd; p != pn; p++) {
    p->revents = 0;
    f = p->fd;
    if (f < 0) continue;
    if (f >= NOFILE) { p->revents = POLLNVAL; continue; }
    switch (xft[f]) {
    case xCONSOLE:
    case xSOCKET:
      f = xfd[f];
      if (p->events & POLLIN)  { phr = &hr; FD_SET(f, &hr); }
      if (p->events & POLLOUT) { phw = &hw; FD_SET(f, &hw); }
      continue;
    default:
      p->revents = POLLNVAL;
    }  
  }

//  if (phr || phw) { XXX
    if (msec >= 0) { t.tv_sec = msec / 1000; t.tv_usec = (msec % 1000) * 1000; }
    if ((r = select(FD_SETSIZE, phr, phw, 0, (msec < 0) ? 0 : &t)) < 0) return r;
//  }
//  else if (msec < 0) return -1;
//  else { if (msec > 0) Sleep(msec); return 0; }

  r = 0;
  for (p = pfd; p != pn; p++) {
    f = p->fd;
    if (f < 0 || f >= NOFILE) continue;
    switch (xft[f]) {
    case xCONSOLE:
    case xSOCKET:
      f = xfd[f];
      if (FD_ISSET(f, &hr)) p->revents |= POLLIN;
      if (FD_ISSET(f, &hw)) p->revents |= POLLOUT;
      if (p->revents) r++;
      continue;
    }
  }
  return r;
}

int xbind(int d, void *a, int sz)
{
  return ((uint)d >= NOFILE) ? -1 : bind(xfd[d], a, sz);
}
int xlisten(int d, int n)
{
  return ((uint)d >= NOFILE) ? -1 : listen(xfd[d], n);
}
int xaccept(int d, void *a, void *b)
{
  int i,r;
  if ((uint)d >= NOFILE) return -1;
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if ((r = accept(xfd[d],a,b)) < 0) return r;
      xft[i] = xSOCKET; xfd[i] = r;
      return i;
    }
  }
  return -1;  
}
int xconnect(int d, void *a, int n)
{
  return ((uint)d >= NOFILE) ? -1 : connect(xfd[d], a, n);
}

#define socket   xsocket
#define bind     xbind
#define listen   xlisten
#define accept   xaccept
#define connect  xconnect
