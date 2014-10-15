// libc.h

enum { EOF = -1, NULL };
enum { S_IFIFO = 0x1000, S_IFCHR = 0x2000, S_IFBLK = 0x3000, S_IFDIR = 0x4000, S_IFREG = 0x8000, S_IFMT = 0xF000 }; // XXX split off into stat.h?
enum { O_RDONLY, O_WRONLY, O_RDWR, O_CREAT = 0x100, O_TRUNC = 0x200 };
enum { SEEK_SET, SEEK_CUR, SEEK_END };
enum { BUFSIZ = 1024, NAME_MAX = 256, PATH_MAX = 256 }; // XXX
enum { POLLIN = 1, POLLOUT = 2, POLLNVAL = 4 };

struct stat { ushort st_dev; ushort st_mode; uint st_ino; uint st_nlink; uint st_size; };
struct pollfd { int fd; short events, revents; };

// intrinsics
void *memcpy() { asm(LL,8); asm(LBL, 16); asm(LCL,24); asm(MCPY); asm(LL,8); }
void *memset() { asm(LL,8); asm(LBLB,16); asm(LCL,24); asm(MSET); asm(LL,8); }
int   memcmp() { asm(LL,8); asm(LBL, 16); asm(LCL,24); asm(MCMP); }
void *memchr() { asm(LL,8); asm(LBLB,16); asm(LCL,24); asm(MCHR); }

// system calls
fork()   { asm(TRAP,S_fork); }
exit()   { asm(LL,8); asm(TRAP,S_exit); }
wait()   { asm(TRAP,S_wait); }
pipe()   { asm(LL,8); asm(TRAP,S_pipe); }
write()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_write); }
read()   { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_read); }
close()  { asm(LL,8); asm(TRAP,S_close); }
kill()   { asm(LL,8); asm(TRAP,S_kill); }
exec()   { asm(LL,8); asm(LBL,16); asm(TRAP,S_exec); } 
open()   { asm(LL,8); asm(LBL,16); asm(TRAP,S_open); } // XXX 3rd arg?
mknod()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_mknod); }
unlink() { asm(LL,8); asm(TRAP,S_unlink); }
fstat()  { asm(LL,8); asm(LBL,16); asm(TRAP,S_fstat); }
link()   { asm(LL,8); asm(LBL,16); asm(TRAP,S_link); }
mkdir()  { asm(LL,8); asm(TRAP,S_mkdir); }
chdir()  { asm(LL,8); asm(TRAP,S_chdir); }
dup2()   { asm(LL,8); asm(LBL,16); asm(TRAP,S_dup2); }
getpid() { asm(TRAP,S_getpid); }
void *sbrk() { asm(LL,8); asm(TRAP,S_sbrk); }
sleep()  { asm(LL,8); asm(TRAP,S_sleep); } // XXX is this seconds? should it be?
uptime() { asm(TRAP,S_uptime); }
lseek()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_lseek); }
mount()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_mount); }
umount() { asm(LL,8); asm(TRAP,S_umount); }
poll()   { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_poll); }

// string routines
int strcmp(char *d, char *s) { for (; *d == *s; d++, s++) if (!*d) return 0; return *d - *s; }
int strlen(char *s) { return memchr(s, 0, -1) - s; }
char *strcpy(char *d, char *s) { return memcpy(d, s, strlen(s)+1); }
char *strcat(char *d, char *s) { memcpy(memchr(d, 0, -1), s, strlen(s)+1); return d; }
int strncmp(char *d, char *s, int n) { while (n > 0) { if (!*d || *d != *s) return *d - *s; n--; d++; s++; } return 0; }
char *strchr(char *s, int c) { return memchr(s, c, strlen(s)); }
// XXX strncpy
// XXX index
// XXX rindex

// XXX wrong for now!
int sscanf(char *s, char *f, int *a) { if (strcmp(f,"%i")) *(double *)a = 2.1; else *a = 1; return 1; }

// a few math for printf
double pow(double x, double y) { asm(LLD,8); asm(LBLD,16); asm(POW); }
double floor(double x) { asm(LLD,8); asm(FLOR); }
double fmod(double x, double y) { asm(LLD,8); asm(LBLD,16); asm(FMOD); }

int vsprintf(char *s, char *f, va_list v)
{
  char *e = s, *p, c, fill, b[BUFSIZ];
  int i, left, fmax, fmin, sign, prec;
  double d;

  while (c = *f++) {
    if (c != '%') { *e++ = c; continue; }
    if (*f == '%') { *e++ = *f++; continue; }
    if (left = (*f == '-')) f++;
    fill = (*f == '0') ? *f++ : ' ';
    fmin = sign = 0; fmax = BUFSIZ; prec = 6;
    if (*f == '*') { fmin = va_arg(v,int); f++; } else while ('0' <= *f && *f <= '9') fmin = fmin * 10 + *f++ - '0';
    if (*f == '.') { if (*++f == '*') { fmax = va_arg(v,int); f++; } else { for (fmax = 0; '0' <= *f && *f <= '9'; fmax = fmax * 10 + *f++ - '0'); prec = fmax; } }
    if (*f == 'l') f++;
    switch (c = *f++) {
    case 0: *e++ = '%'; *e = 0; return e - s;
    case 'c': fill = ' '; i = (*(p = b) = va_arg(v,int)) ? 1 : 0; break;
    case 's': fill = ' '; if (!(p = va_arg(v,char *))) p = "(null)"; if ((i = strlen(p)) > fmax) i = fmax; break;
    case 'u': i = va_arg(v,int); goto c1;
    case 'd': if ((i = va_arg(v,int)) < 0) { sign = 1; i = -i; } c1: p = b + BUFSIZ-1; do { *--p = ((uint)i % 10) + '0'; } while (i = (uint)i / 10); i = (b + BUFSIZ-1) - p; break;
    case 'o': i = va_arg(v,int); p = b + BUFSIZ-1; do { *--p = (i & 7) + '0'; } while (i = (uint)i >> 3); i = (b + BUFSIZ-1) - p; break;
    case 'p': fill = '0'; fmin = 8; c = 'x';
    case 'x': case 'X': c -= 33; i = va_arg(v,int); p = b + BUFSIZ-1; do { *--p = (i & 15) + ((i & 15) > 9 ? c : '0'); } while (i = (uint)i >> 4); i = (b + BUFSIZ-1) - p; break;
    case 'e': case 'E': e1: //XXX
    case 'f': if ((d = va_arg(v,double)) < 0) { sign = 1; d = -d; } d = d * pow(10.0,prec); p = b + BUFSIZ-1; i = prec;
//            while (i >= 0 || d > 0.0) { if (!i-- && prec) *--p = '.'; *--p = '0' + ((d > 1.0e15) ? 0 : ((int)fmod(d+0.5,10.0))); d = floor(d * 0.1); }  XXX
              while (i >= 0 || d > 0.0) { if (!i-- && prec) *--p = '.'; *--p = '0' + ((d > 1000000000000000.0) ? 0 : ((int)fmod(d+0.5,10.0))); d = floor(d * 0.1); }
              i = (b + BUFSIZ-1) - p; break;
    case 'g': case 'G': c -= 2; if ((d = va_arg(v,double)) < 0) { sign = 1; d = -d; } if (d < 0.0001 || d >= pow(10.0,prec)) goto e1;
              p = "<g>"; i = 3; break; // XXX
//    case 'e': case 'E': e1: p = "<e>"; i = 3; break; // XXX see above 'f'
    default: *e++ = c; continue;
    }
    fmin -= i + sign;
    if (sign && fill == '0') *e++ = '-';
    if (!left && fmin > 0) { memset(e, fill, fmin); e += fmin; }
    if (sign && fill == ' ') *e++ = '-';
    memcpy(e, p, i); e += i;
    if (left && fmin > 0) { memset(e, fill, fmin); e += fmin; }
  }
  *e = 0;
  return e - s;
}

int sprintf(char *s, char *f, ...) { va_list v; va_start(v, f); return vsprintf(s, f, v); }
int printf(char *f, ...) { char s[BUFSIZ]; va_list v; va_start(v, f); return write(1, s, vsprintf(s, f, v)); }
int vprintf(char *f, va_list v) { char s[BUFSIZ]; return write(1, s, vsprintf(s, f, v)); }
int dprintf(int d, char *f, ...) { char s[BUFSIZ]; va_list v; va_start(v, f); return write(d, s, vsprintf(s, f, v)); }
int vdprintf(int d, char *f, va_list v) { char s[BUFSIZ]; return write(d, s, vsprintf(s, f, v)); }

void *malloc(uint n) { int i = sbrk(n); return (i == -1) ? 0 : i; } // XXX placeholder, see mem.h
void free(void *p) { } // XXX

int atoi(char *s)
{
  int i = 0, n; char c;
  while ((c = *s++) == ' ' || c == '\t');
  if (c == '-') n = -1; else { n = 1; if (c != '+') s--; }
  while ((c = *s++) >= '0' && c <= '9') i = i*10 + c - '0';
  return n * i;
}

int stat(char *n, struct stat *s) { int f, r; if ((f = open(n, O_RDONLY)) < 0) return -1; r = fstat(f, s); close(f); return r; }
