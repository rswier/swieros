// dir.h

enum { DPOOL_SZ = 8 };

struct dirent { char d_name[NAME_MAX]; };
typedef struct { int fd, ref, ino; struct dirent de; } DIR;

DIR *opendir(char *name)
{
  static DIR pool[DPOOL_SZ];
  DIR *d;
  for (d = pool; d < pool + DPOOL_SZ; d++)
    if (!d->ref) { if ((d->fd = open(name, O_RDONLY)) < 0) return 0; d->ref = 1; return d; }
  return 0;
}  
struct dirent *readdir(DIR *d)
{
  for (;;) {
    if (read(d->fd, &d->ino, NAME_MAX) < NAME_MAX) return 0; 
    if (d->ino) return &d->de;
  }
}
int closedir(DIR *d) { close(d->fd); d->ref = 0; return 0; }
void rewinddir(DIR *d) { lseek(d->fd, 0, SEEK_SET); }

char *getcwd(char *path, int size)
{
  char cbuf[PATH_MAX], *c, *p;
  int i, ino;
  struct stat st, dt, root;
  struct dirent *dir;
  DIR *d;

  if (!path || size < 2 || stat("/", &root) || stat(".", &st)) return 0;
  c = cbuf;
  *(p = path + size - 1) = 0;
  while (st.st_ino != root.st_ino) {
    ino = st.st_ino;
    *c = c[1] = '.'; c[2] = '/'; c += 3; *c = 0;
    if (stat(cbuf, &st)) return 0;
    if (ino == st.st_ino) break;
    if (!(d = opendir(cbuf))) return 0;
    do {
      do { if (!(dir = readdir(d))) { closedir(d); return 0; } }
      while (dir->d_name[0] == '.' && (!dir->d_name[1] || (dir->d_name[1] == '.' && !dir->d_name[2])));
      strcpy(c, dir->d_name);
      if (stat(cbuf, &dt)) { closedir(d); return 0; } // XXX vs lstat?
    } while (dt.st_ino != ino);
    closedir(d);
    p -= (i = strlen(c));
    if (p <= path) return 0;
    memcpy(p, c, i);
    *--p = '/';
  }
  if (!*p) { *path = '/'; path[1] = 0; }
  else if (p > path) strcpy(path, p);
  return path;
}
