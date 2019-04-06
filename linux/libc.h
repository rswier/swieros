// linux/libc.h
// Allows a few specific programs to compile and run under linux.

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <memory.h>
#include <stdarg.h>
#include <termios.h>
#include <sys/stat.h>
#include <dirent.h>

#define NOFILE 16 // XXX subject to change

#undef NAME_MAX
#undef PATH_MAX
#define NAME_MAX 256
#define PATH_MAX 256

enum { xCLOSED, xCONSOLE, xFILE, xSOCKET, xDIR };
int xfd[NOFILE];
int xft[NOFILE];
DIR *xdir[NOFILE];

char *pesc = 0;

int xopen(char *fn, int mode)
{
  int i,d;
  DIR *dir;
  struct stat hs; int r;
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if (!(mode & O_CREAT) && !stat(fn, &hs) && S_ISDIR(hs.st_mode)) {
        if (!(dir = opendir(fn))) return -1;
        xft[i] = xDIR;
        xdir[i] = dir;
      } else {
        if ((d = open(fn, mode, S_IRWXU)) < 0) return d;
        xft[i] = xFILE;
        xfd[i] = d;
      }
      return i;
    }
  }
  return -1;
}
int xclose(int d)
{
  int r;
  if ((uint)d >= NOFILE) return -1;
  switch (xft[d]) {
  case xSOCKET:
  case xFILE: r = close(xfd[d]); break;
  case xDIR: closedir(xdir[d]); r = 0; break;
  }
  xfd[d] = -1; xft[d] = xCLOSED;
  return r;
}
int xread(int d, void *b, int n)
{
  struct dirent *de;
  int c;

  if ((uint)d >= NOFILE) return -1;
  switch (xft[d]) {
  case xSOCKET: return read(xfd[d], b, n);
  case xCONSOLE: return read(0,b,1);
  case xDIR:
    if (n != NAME_MAX) return 0;
    if (!(de = readdir(xdir[d]))) return 0;
    n = 1; memcpy(b, &n, 4);
    strncpy((char *)b+4, de->d_name, NAME_MAX-4);
    return NAME_MAX; // XXX hardcoded crap
  case xFILE: return read(xfd[d], b, n);
  }
  return -1;
}
int xwrite(int d, void *b, int n)
{
  if ((uint)d >= NOFILE) return -1;
  switch (xft[d]) {
  case xSOCKET:
  case xFILE: // return write(xfd[d], b, n);
  case xCONSOLE: return write(xfd[d], b, n); // return write(1, b, n); XXX
  }
  return -1;
}
int xlseek(int d, int offset, int whence)
{
  return ((uint)d >= NOFILE) ? -1 : lseek(xfd[d], offset, whence);
}
int xprintf(char *f, ...)
{
  static char buf[4096]; va_list v; int n;
  va_start(v, f);
  n = vsprintf(buf, f, v);  // XXX should be my version!
  va_end(v);
  return xwrite(1, buf, n);
}
int xvprintf(char *f, va_list v)
{
  static char buf[4096];
  return xwrite(1, buf, vsprintf(buf, f, v)); // XXX should be my version!
}
int xdprintf(int d, char *f, ...)
{
  static char buf[4096]; va_list v; int n;
  va_start(v, f);
  n = vsprintf(buf, f, v);
  va_end(v);
  return xwrite(d, buf, n);
}
int xvdprintf(int d, char *f, va_list v)
{
  static char buf[4096];
  return xwrite(d, buf, vsprintf(buf, f, v)); // XXX
}

struct xstat {
  ushort st_dev;   // device number
  ushort st_mode;  // type of file
  uint   st_ino;   // inode number on device
  uint   st_nlink; // number of links to file
  uint   st_size;  // size of file in bytes
};
int xfstat(int d, struct xstat *s)
{
  struct stat hs; int r;
  if ((uint)d >= NOFILE) return -1;
  if (xft[d] == xDIR) {
    s->st_mode  = S_IFDIR;
    s->st_dev   = 0;
    s->st_ino   = 0;
    s->st_nlink = 0;
    s->st_size  = 0;
    r = 0;
  } else if (!(r = fstat(xfd[d], &hs))) {
    s->st_mode  = S_IFREG;
    s->st_dev   = hs.st_dev;
    s->st_ino   = hs.st_ino;
    s->st_nlink = hs.st_nlink;
    s->st_size  = hs.st_size;
  }
  return r;
}
int xstat(char *file, struct xstat *s)
{
  struct stat hs; int r;
  if (!(r = stat(file, &hs))) {
    s->st_mode  = hs.st_mode;
    s->st_dev   = hs.st_dev;
    s->st_ino   = hs.st_ino;
    s->st_nlink = hs.st_nlink;
    s->st_size  = hs.st_size;
  }
  return r;
}
void *xsbrk(int i)
{
  void *p; static char *brk = 0;
  if (!i) return (void *)brk;
  if (i < 0) { printf("sbrk(i<0) not implemented\n"); exit(-1); }
  if (p = malloc(i)) { memset(p, 0, i); brk += i; return p; } // XXX memset is probably redundant since we never reallocate
  return (void *)-1;
}
int xmkdir(char *path)
{
  return mkdir(path, 0644);
}

int xfork(void) { printf("fork() not implemented\n"); exit(-1); }
int xwait(void) { printf("wait() not implemented\n"); exit(-1); }
int xpipe(int *fd) { printf("pipe() not implemented\n"); exit(-1); }
int xkill(int pid) { printf("kill() not implemented\n"); exit(-1); }
int xexec(char *path, char **argv) { printf("exec() not implemented\n"); exit(-1); }
int xmknod(char *path, int mode, int dev) { printf("mknod() not implemented\n"); exit(-1); }
int xlink(char *old, char *new) { printf("link() not implemented\n"); exit(-1); }
int xgetpid(void) { printf("getpid() not implemented\n"); exit(-1); }
int xsleep(int n) { printf("sleep() not implemented\n"); exit(-1); }
int xuptime(void) { printf("uptime() not implemented\n"); exit(-1); }
int xmount(char *spec, char *dir, int rwflag) { printf("mount() not implemented\n"); exit(-1); }
int xumount(char *spec) { printf("umount() not implemented\n"); exit(-1); }

void xexit(int rc)
{
  struct termios sttybuf;
  tcgetattr(0,&sttybuf);
  sttybuf.c_lflag |= ECHO | ICANON;
  tcsetattr(0,TCSANOW,&sttybuf);
  exit(rc);
}

int main(int argc, char *argv[])
{
  extern int xmain();
  struct termios sttybuf;
  int i;
  tcgetattr(0,&sttybuf);
  sttybuf.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(0,TCSANOW,&sttybuf);
  for (i=0; i<3;      i++) { xfd[i] =  i; xft[i] = xCONSOLE; }
  for (i=3; i<NOFILE; i++) { xfd[i] = -1; xft[i] = xCLOSED;  }
  xexit(xmain(argc, argv));
}

#define printf   xprintf
#define vprintf  xvprintf
#define dprintf  xdprintf
#define vdprintf xvdprintf
#define open     xopen
#define close    xclose
#define read     xread
#define write    xwrite
#define lseek    xlseek
#define stat     xstat
#define fstat    xfstat
#define mkdir    xmkdir

#define fork     xfork
#define wait     xwait
#define pipe     xpipe
#define kill     xkill
#define exec     xexec
#define mknod    xmknod
#define link     xlink
#define getpid   xgetpid
#define sleep    xsleep
#define uptime   xuptime
#define mount    xmount
#define umount   xumount

#define exit     xexit
#define main     xmain
#define sbrk     xsbrk
