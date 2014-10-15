// mingw/net.h
// Allows a few specific programs to compile and run under mingw.

#include <WinSock2.h>

int (WSAAPI *_WSAStartup)(ushort wVersionRequired, LPWSADATA lpWSAData);
int (WSAAPI *_socket)(int af, int type, int protocol);
int (WSAAPI *_connect)(int s, const struct sockaddr *name, int namelen);
int (WSAAPI *_closesocket)(int s);
int (WSAAPI *_send)(int s, const char *buf, int len, int flags);
int (WSAAPI *_recv)(int s, char *buf, int len, int flags);
int (WSAAPI *_accept)(int s, struct sockaddr *addr, int *addrlen);
int (WSAAPI *_bind)(int s, const struct sockaddr *name, int namelen);
int (WSAAPI *_listen)(int s, int backlog);
int (WSAAPI *_select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout);
int (WSAAPI *_WSAFDIsSet)(int s, fd_set *fd);
uint (WSAAPI *_htonl)(uint n);
ushort (WSAAPI *_htons)(ushort n);

void initsock()
{
  WSADATA wsa_data; HMODULE dll;
  if (!(dll = LoadLibrary("WS2_32"))) { printf("LoadLibrary(WS2_32)\n"); exit(9); }
  if (!(_WSAStartup  = (void *) GetProcAddress(dll, "WSAStartup")))   { printf("LoadLibrary(WSAStartup)\n");   exit(9); }
  if (!(_socket      = (void *) GetProcAddress(dll, "socket")))       { printf("LoadLibrary(socket)\n");       exit(9); }
  if (!(_connect     = (void *) GetProcAddress(dll, "connect")))      { printf("LoadLibrary(connect)\n");      exit(9); }
  if (!(_closesocket = (void *) GetProcAddress(dll, "closesocket")))  { printf("LoadLibrary(closesocket)\n");  exit(9); }
  if (!(_send        = (void *) GetProcAddress(dll, "send")))         { printf("LoadLibrary(send)\n");         exit(9); }
  if (!(_recv        = (void *) GetProcAddress(dll, "recv")))         { printf("LoadLibrary(recv)\n");         exit(9); }
  if (!(_accept      = (void *) GetProcAddress(dll, "accept")))       { printf("LoadLibrary(accept)\n");       exit(9); }
  if (!(_bind        = (void *) GetProcAddress(dll, "bind")))         { printf("LoadLibrary(bind)\n");         exit(9); }
  if (!(_listen      = (void *) GetProcAddress(dll, "listen")))       { printf("LoadLibrary(listen)\n");       exit(9); }
  if (!(_select      = (void *) GetProcAddress(dll, "select")))       { printf("LoadLibrary(select)\n");       exit(9); }
  if (!(_htons       = (void *) GetProcAddress(dll, "htons")))        { printf("LoadLibrary(htons)\n");        exit(9); }
  if (!(_htonl       = (void *) GetProcAddress(dll, "htonl")))        { printf("LoadLibrary(htonl)\n");        exit(9); }
  if (!(_WSAFDIsSet  = (void *) GetProcAddress(dll, "__WSAFDIsSet"))) { printf("LoadLibrary(__WSAFDIsSet)\n"); exit(9); }
  if (_WSAStartup(MAKEWORD(2,0), &wsa_data) == -1) { printf("WSAStartup()\n"); exit(9); }
}

int xswrite(int d, void *b, int n) { return _send(d, b, n, 0); }
int xsread(int d, void *b, int n) { return _recv(d, b, n, 0); }
int xsclose(int d) { return _closesocket(d); }
int xsocket(int family, int ty, int protocol)
{
  int i,d; static int init = 0;
  if (!init) {
    initsock();
    pxswrite = xswrite;
    pxsread = xsread;
    pxsclose = xsclose;
    init = 1;
  }
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if ((d = _socket(family, ty, protocol)) < 0) return d;
      xft[i] = xSOCKET; xfd[i] = d; //printf("socket()%d=%d\n",d,i);
      return i;
    }
  }
  return -1;  
}

struct pollfd { int fd; short events, revents; };
enum { POLLIN = 1, POLLOUT = 2, POLLNVAL = 4 };  
int poll(struct pollfd *pfd, uint n, int msec)
{
  struct pollfd *p, *pn = &pfd[n];
  int f, r, kb, k;
  fd_set hr, hw, *phr, *phw; struct timeval t; 
    
  for (;;) {
    kb = k = 0; phr = phw = 0;
    FD_ZERO(&hr); FD_ZERO(&hw);
    for (p = pfd; p != pn; p++) {
      p->revents = 0;
      f = p->fd;
      if (f < 0) continue;
      if (f >= NOFILE) { p->revents = POLLNVAL; continue; }
      switch (xft[f]) {
      case xCONSOLE: if (p->events & POLLIN) { kb = 1; if (pesc || kbhit()) { k = 1; msec = 0; } } continue;
      case xSOCKET:
        f = xfd[f];
        if (p->events & POLLIN)  { phr = &hr; FD_SET(f, &hr); }
        if (p->events & POLLOUT) { phw = &hw; FD_SET(f, &hw); }
        continue;
      default:
        p->revents = POLLNVAL;
      }  
    }

    // polling the keyboard every 100ms is good enough for the few apps that need it.
    if (phr || phw) {
      if (kb && (msec < 0 || msec > 100)) { t.tv_sec = 0; t.tv_usec = 100000; if (msec > 100) msec -= 100; }
      else if (msec >= 0) { t.tv_sec = msec / 1000; t.tv_usec = (msec % 1000) * 1000; msec = 0; }
      if ((r = _select(FD_SETSIZE, phr, phw, 0, (msec < 0 && !kb) ? 0 : &t)) < 0) return r;
    }
    else if (kb) {
      if (msec < 0) Sleep(100);
      else if (msec > 100) { Sleep(100); msec -= 100; }
      else if (msec) { Sleep(msec); msec = 0; }
    }
    else if (msec < 0) return -1;
    else { if (msec > 0) Sleep(msec); return 0; }

    r = 0;
    for (p = pfd; p != pn; p++) {
      f = p->fd;
      if (f < 0 || f >= NOFILE) continue;
      switch (xft[f]) {
      case xCONSOLE: if ((p->events & POLLIN) && (k || kbhit())) { p->revents = POLLIN; r++; } continue;
      case xSOCKET:
        f = xfd[f];
        if (_WSAFDIsSet(f, &hr)) p->revents |= POLLIN;
        if (_WSAFDIsSet(f, &hw)) p->revents |= POLLOUT;
        if (p->revents) r++;
        continue;
      }
    }
    if (r || !msec) return r;
  }
}

int xbind(int d, void *a, int sz)
{
  return ((uint)d >= NOFILE) ? -1 : _bind(xfd[d], a, sz);
}
int xlisten(int d, int n)
{
  return ((uint)d >= NOFILE) ? -1 : _listen(xfd[d], n);
}
int xaccept(int d, void *a, void *b)
{
  int i,r;
  if ((uint)d >= NOFILE) return -1;
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if ((r = _accept(xfd[d],a,b)) < 0) return r;
      xft[i] = xSOCKET; xfd[i] = r;
      return i;
    }
  }
  return -1;  
}
int xconnect(int d, void *a, int n)
{
  return ((uint)d >= NOFILE) ? -1 : _connect(xfd[d], a, n);
}

#define socket   xsocket
#define bind     xbind
#define listen   xlisten
#define accept   xaccept
#define connect  xconnect
#define htons    _htons
#define htonl    _htonl
