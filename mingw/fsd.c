// fsd -- win32 file server daemon

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#define NAME_MAX 256

enum { M_OPEN, M_CLOSE, M_READ, M_WRITE, M_SEEK, M_FSTAT, M_SYNC };

// M_LINK, M_UNLINK, M_MKDIR

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

int debug;
int verbose;
int sync; // XXX

int sd = -1;
int fd = -1;
DIR *dir;
char *s;
struct xstat {
  ushort st_dev;   // device number
  ushort st_mode;  // type of file
  uint   st_ino;   // inode number on device
  uint   st_nlink; // number of links to file
  uint   st_size;  // size of file in bytes
};

void fatal(char *s)
{
  printf("fatal error: %s\n",s);
  exit(-1);
}

void tx(void *p, uint n)
{
  int r;
  while (n) {
    if ((r = send(sd, p, n, 0)) <= 0) fatal("tx()");
    n -= r;
    p += r; 
  }
}

void rx(void *p, int n)
{
  int r;
  while (n) {
    if ((r = recv(sd, p, n, 0)) <= 0) { if (verbose) printf("Connection closed\n"); exit(0); }
    n -= r;
    p += r; 
  }
}

ssize(int n)
{
  static int sn = 0;
  n = (n + 4095) & -4096;
  if (n > sn) {
    sn = n;
    if (s) free(s);
    s = malloc(n);
    if (!s) fatal("ssize() malloc");
  }
}

void rxmsgs(void)
{
  int h[3], n, r; struct stat hs; struct xstat xs;
  struct dirent *de;
  
  for (;;) {
    rx(&n,4);
    switch (n) {
    case M_OPEN:
      if (fd >= 0 || dir) fatal("open");
      rx(h,12); ssize(h[2]);
      rx(s,h[2]);
//      sync = (h[0] & O_SYNC);
      sync = 0;
      if (!(h[0] & O_CREAT) && !stat(s, &hs) && S_ISDIR(hs.st_mode)) {
        r = (dir = opendir(s)) ? 0 : -1;
        if (debug) printf("%d = opendir(%s)\n",r,s);
      } else {
        h[1] = S_IRWXU; // XXX
        fd = open(s, h[0] | O_BINARY, h[1]); // XXX third arg?
        if (debug) printf("%d = open(%s, %d, %d)\n",fd,s,h[0],h[1]);
        r = (fd >= 0) ? 0 : -1;
      }
      tx(&r,4);
      break;
    case M_CLOSE:
      if (fd >= 0) {
        close(fd);
        if (debug) printf("close(%d)\n",fd);
        fd = -1;
      } else if (dir) {
        closedir(dir);
        if (debug) printf("closedir()\n");
        dir = 0;
      } else fatal("close");
      break;
    case M_READ:
      if (fd >= 0) {
        rx(&n,4); ssize(n);
        r = read(fd, s, n);
        if (debug) printf("%d = read(%d, s, %d)\n", r, fd, n);
        tx(&r,4);
        if ((uint)r <= n) tx(s, r);
      } else if (dir) {
        rx(&n,4); ssize(n);
        r = 0;
        if (n == NAME_MAX && (de = readdir(dir))) {
          r = 1; memcpy(s, &r, 4);
          strncpy(s + 4, de->d_name, NAME_MAX - 4);
          r = NAME_MAX;
        }
        if (debug) printf("%d = readdir(%d)\n", r, n);
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
          xs.st_mode  = hs.st_mode;
          xs.st_dev   = hs.st_dev;
          xs.st_ino   = hs.st_ino;
          xs.st_nlink = hs.st_nlink;
          xs.st_size  = hs.st_size;
          tx(&xs, sizeof(xs));
        }
      } else if (dir) {
        r = 0;
        if (debug) printf("%d = fstat(dir)\n", r);
        tx(&r,4);
        xs.st_mode  = S_IFDIR;
        xs.st_dev   = 0;
        xs.st_ino   = 0;
        xs.st_nlink = 0;
        xs.st_size  = 0;
        tx(&xs, sizeof(xs));
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
  WSADATA info; int i, ld;
  char cmdline[256];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  struct sockaddr_in addr;
  int port = 5003;
  int server = 1;

  if (WSAStartup(MAKEWORD(2,0),&info) != 0) fatal("WSAStartup()");

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-v")) verbose = 1; else
    if (!strcmp(argv[i],"-I")) server = 0; else
    if (!strcmp(argv[i],"-d")) debug = verbose = 1; else
    if (!strcmp(argv[i],"-p") && i+1 < argc) port = atoi(argv[++i]); else
    fatal("usage: fsrv [-v] [-d] [-p port]");
  }
  
  if (server) {
    if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) fatal("socket()");
    i = 1; setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ld, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) { closesocket(ld); fatal("bind()"); }
    if (listen(ld, 1) < 0) { closesocket(ld); fatal("listen()"); }

    if (verbose) printf("Listening on port %d...\n", port) ;
    for (;;) {
      if ((sd = accept(ld, 0, 0)) < 0) fatal("accept()");
      if (verbose) printf("Connection accepted\n") ;
      i = 1; setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *) &i, sizeof(i));
      
      strcpy(cmdline, argv[0]);
      strcat(cmdline, " -I");
      if (debug) strcat(cmdline, " -d");
      if (verbose) strcat(cmdline, " -v");
            
      memset(&si, 0 , sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.hStdInput = (HANDLE)sd;
      si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
      si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
      si.dwFlags = STARTF_USESTDHANDLES;
      
      if (!CreateProcess(NULL, // application name
          cmdline, // command line
          NULL,    // process security attributes
          NULL,    // thread security attributes
          TRUE,    // inherit handles
          0,       // creation flags
          NULL,    // environment
          NULL,    // current directory
          &si,     // startup info
          &pi)) fatal("CreateProcess()");

      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);

      closesocket(sd);      
    }
  } else {
    sd = (SOCKET) GetStdHandle(STD_INPUT_HANDLE);
    rxmsgs();
  }
  return 0;
}
