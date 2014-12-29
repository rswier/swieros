// mingw/libc.h
// Allows a few specific programs to compile and run under mingw.

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <memory.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#define NOFILE 16 // XXX subject to change
#define NAME_MAX 256
#undef PATH_MAX
#define PATH_MAX 256

enum { xCLOSED, xCONSOLE, xFILE, xSOCKET, xDIR };
int xfd[NOFILE];
int xft[NOFILE];
int (*pxswrite)(int, void *, int);
int (*pxsread)(int, void *, int);
int (*pxsclose)(int);

char *pesc = 0;

int xopen(char *fn, int mode)
{
  int i,d;
  struct stat hs; int r;
  for (i=0;i<NOFILE;i++) {
    if (xft[i] == xCLOSED) {
      if (!(mode & O_CREAT) && !stat(fn, &hs) && S_ISDIR(hs.st_mode)) {
        if (!(d = (int)opendir(fn))) return -1;
        xft[i] = xDIR;
      } else {
        if ((d = open(fn, mode | O_BINARY, S_IRWXU)) < 0) return d;
        xft[i] = xFILE;
      }
      xfd[i] = d;
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
  case xSOCKET: r = pxsclose(xfd[d]); break;
  case xFILE: r = close(xfd[d]); break;
  case xDIR: closedir((DIR*)xfd[d]); r = 0; break;
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
  case xSOCKET: return pxsread(xfd[d], b, n);
  case xCONSOLE:
    if (pesc) { *(char *)b = *pesc++; if (!*pesc) pesc = 0; return 1; }
    c = getch();
    if (c && c != 0xE0) { *(char*)b = c; return 1; }
    switch (getch())
    {
    case 0x48: pesc = "[A"; break; // UP
    case 0x50: pesc = "[B"; break; // DOWN
    case 0x4D: pesc = "[C"; break; // RIGHT
    case 0x4B: pesc = "[D"; break; // LEFT
    case 0x47: pesc = "[1~"; break; // HOME
    case 0x52: pesc = "[2~"; break; // INS
    case 0x53: pesc = "[3~"; break; // DEL
    case 0x4F: pesc = "[4~"; break; // END
    case 0x49: pesc = "[5~"; break; // PGUP
    case 0x51: pesc = "[6~"; break; // PGDN
      
    case 0x3B: pesc = "[11~"; break; // F1
    case 0x3C: pesc = "[12~"; break; // F2
    case 0x3D: pesc = "[13~"; break; // F3
    case 0x3E: pesc = "[14~"; break; // F4
    case 0x3F: pesc = "[15~"; break; // F5
    case 0x40: pesc = "[17~"; break; // F6
    case 0x41: pesc = "[18~"; break; // F7
    case 0x42: pesc = "[19~"; break; // F8
    case 0x43: pesc = "[20~"; break; // F9
    case 0x44: pesc = "[21~"; break; // F10
    case 0x85: pesc = "[23~"; break; // F11

    case 0x54: pesc = "[11;2~"; break; // SHIFT F1
    case 0x55: pesc = "[12;2~"; break; // SHIFT F2
    case 0x56: pesc = "[13;2~"; break; // SHIFT F3
    case 0x57: pesc = "[14;2~"; break; // SHIFT F4
    case 0x58: pesc = "[15;2~"; break; // SHIFT F5
    case 0x59: pesc = "[17;2~"; break; // SHIFT F6
    case 0x5A: pesc = "[18;2~"; break; // SHIFT F7
    case 0x5B: pesc = "[19;2~"; break; // SHIFT F8
    case 0x5C: pesc = "[20;2~"; break; // SHIFT F9
    case 0x5D: pesc = "[21;2~"; break; // SHIFT F10
    case 0x87: pesc = "[23;2~"; break; // SHIFT F11
    case 0x88: pesc = "[24;2~"; break; // SHIFT F12

    case 0x8D: pesc = "[1;5A"; break; // CTRL UP
    case 0x91: pesc = "[1;5B"; break; // CTRL DOWN
    case 0x74: pesc = "[1;5C"; break; // CTRL RIGHT
    case 0x73: pesc = "[1;5D"; break; // CTRL LEFT
    case 0x77: pesc = "[1;5~"; break; // CTRL HOME
    case 0x92: pesc = "[2;5~"; break; // CTRL INS
    case 0x93: pesc = "[3;5~"; break; // CTRL DEL
    case 0x75: pesc = "[4;5~"; break; // CTRL END
    case 0x86: pesc = "[5;5~"; break; // CTRL PGUP
    case 0x76: pesc = "[6;5~"; break; // CTRL PGDN

    case 0x5E: pesc = "[11;5~"; break; // CTRL F1
    case 0x5F: pesc = "[12;5~"; break; // CTRL F2
    case 0x60: pesc = "[13;5~"; break; // CTRL F3
    case 0x61: pesc = "[14;5~"; break; // CTRL F4
    case 0x62: pesc = "[15;5~"; break; // CTRL F5
    case 0x63: pesc = "[17;5~"; break; // CTRL F6
    case 0x64: pesc = "[18;5~"; break; // CTRL F7
    case 0x65: pesc = "[19;5~"; break; // CTRL F8
    case 0x66: pesc = "[20;5~"; break; // CTRL F9
    case 0x67: pesc = "[21;5~"; break; // CTRL F10
    case 0x89: pesc = "[23;5~"; break; // CTRL F11
    case 0x8A: pesc = "[24;5~"; break; // CTRL F12
    }
    *(char *)b = '\e';
    return 1;
  case xDIR:
    if (n != NAME_MAX) return 0;
    if (!(de = readdir((DIR*)xfd[d]))) return 0;
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
  case xSOCKET: return pxswrite(xfd[d], b, n);
  case xCONSOLE:
  case xFILE: return write(xfd[d], b, n);
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
void *sbrk(int i)
{
  void *p; static brk = 0;
  if (!i) return (void *)brk;
  if (i < 0) { printf("sbrk(i<0) not implemented\n"); exit(-1); }
  if (p = malloc(i)) { memset(p, 0, i); brk += i; return p; } // XXX memset is probably redundant since we never reallocate
  return (void *)-1;
}

int fork(void) { printf("fork() not implemented\n"); exit(-1); }
int wait(void) { printf("wait() not implemented\n"); exit(-1); }
int pipe(int *fd) { printf("pipe() not implemented\n"); exit(-1); }
int kill(int pid) { printf("kill() not implemented\n"); exit(-1); }
int exec(char *path, char **argv) { printf("exec() not implemented\n"); exit(-1); }
int mknod(char *path, int mode, int dev) { printf("mknod() not implemented\n"); exit(-1); }
int link(char *old, char *new) { printf("link() not implemented\n"); exit(-1); }
int getpid(void) { printf("getpid() not implemented\n"); exit(-1); }
int xsleep(int n) { printf("sleep() not implemented\n"); exit(-1); }
int uptime(void) { printf("uptime() not implemented\n"); exit(-1); }
int mount(char *spec, char *dir, int rwflag) { printf("mount() not implemented\n"); exit(-1); }
int umount(char *spec) { printf("umount() not implemented\n"); exit(-1); }

int main(int argc, char *argv[])
{
  extern int xmain();
  int i;
  for (i=0; i<3;      i++) { xfd[i] =  i; xft[i] = xCONSOLE; } 
  for (i=3; i<NOFILE; i++) { xfd[i] = -1; xft[i] = xCLOSED;  } 
  xmain(argc, argv);
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
#define main     xmain
#define sleep    xsleep