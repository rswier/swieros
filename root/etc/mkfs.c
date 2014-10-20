// mkfs.c - make file system

#include <u.h>
#include <libc.h>
#include <dir.h>

enum {
  BUFSZ   = 16*4096,     // bitmap size
  DIRSIZ  = 252,
  NDIR    = 480,         // 1.9 MB
  NIDIR   = 512,         //   2 GB
  NIIDIR  = 8,           //  32 GB
  NIIIDIR = 4,           //  16 TB
};

struct dinode {          // 4K disk inode structure
  ushort mode;           // file mode
  uint nlink;            // number of links to inode in file system
  uint size;             // size of file
  uint pad[17];
  uint dir[NDIR];        // data block addresses
  uint idir[NIDIR];      // 2 GB max file size
  uint iidir[NIIDIR];    // not yet implemented
  uint iiidir[NIIIDIR];  // not yet implemented
};

struct direct { // disk directory entry structure
  uint d_ino;
  char d_name[DIRSIZ];
};

uchar buf[BUFSZ];
uint bn;
int disk;

void xstrncpy(char *s, char *t, int n) // no return value unlike strncpy
{
  while (n-- > 0 && (*s++ = *t++));
  while (n-- > 0) *s++ = 0;
}

void write_disk(void *b, uint n)
{
  uint i;
  while (n) {
    if ((i = write(disk, b, n)) < 0) { dprintf(2, "write(%d) failed\n",n); exit(-1); }
    b += i; n -= i;
  }
}

void write_meta(uint size, uint mode, uint nlink)
{
  uint i, b, dir, idir;
  struct dinode inode;
  static uint iblock[1024];
    
  // compute blocks, direct, and indirect
  b = (size + 4095) / 4096;
  dir = (b > NDIR) ? NDIR : b;
  idir = (b + 1023 - NDIR) / 1024;

  // write inode
  memset(&inode, 0, 4096);
  inode.mode = mode;
  inode.nlink = nlink;
  inode.size = size;
  bn++;
  for (i=0; i<dir;  i++) inode.dir[i] = bn + idir + i;
  for (i=0; i<idir; i++) inode.idir[i] = bn++;
  write_disk(&inode, 4096);

  // write indirect blocks
  b += bn;
  bn += dir;
  while (bn < b) {
    for (i=0; i<1024; i++) iblock[i] = (bn < b) ? bn++ : 0;
    write_disk(iblock, 4096);
  }
}

add_dir(uint parent, struct direct *sp)
{
  uint size, dsize, dseek, nlink = 2;
  int f, n, i;
  struct direct *de, *p;
  DIR *d;
  struct dirent *dp;
  struct stat st;
  static uchar zeros[4096];
    
  // build directory
  de = sp;
  d = opendir(".");
  sp->d_ino = bn;     xstrncpy(sp->d_name, ".",  DIRSIZ); sp++;
  sp->d_ino = parent; xstrncpy(sp->d_name, "..", DIRSIZ); sp++;
  while (dp = readdir(d)) {
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") || strlen(dp->d_name) > DIRSIZ) continue;
    if (stat(dp->d_name, &st)) { dprintf(2, "stat(%s) failed\n", dp->d_name); exit(-1); }
    if ((st.st_mode & S_IFMT) == S_IFREG) sp->d_ino = st.st_size;
    else if ((st.st_mode & S_IFMT) == S_IFDIR) { sp->d_ino = -1; nlink++; }
    else continue;
    xstrncpy(sp->d_name, dp->d_name, DIRSIZ);
    sp++;
  }
  closedir(d);
  parent = bn;
  
  // write inode
  write_meta(dsize = (uint)sp - (uint)de, S_IFDIR, nlink);  
  dseek = (bn - ((dsize + 4095) / 4096)) * 4096;

  // write directory
  write_disk(de, dsize);
  if (dsize & 4095) write_disk(zeros, 4096 - (dsize & 4095));

  // add directory contents
  for (p = de + 2; p < sp; p++) {
    size = p->d_ino; p->d_ino = bn;
    if (size == -1) { // subdirectory
      chdir(p->d_name);
      add_dir(parent, sp);
      chdir("..");
    } else { // file
      write_meta(size, S_IFREG, 1);
      if (size) {
        if ((f = open(p->d_name, O_RDONLY)) < 0) { dprintf(2, "open(%s) failed\n", p->d_name); exit(-1); }
        for (n = size; n; n -= i) {
          if ((i = read(f, buf, (n > BUFSZ) ? BUFSZ : n)) < 0) { dprintf(2, "read(%s) failed\n", p->d_name); exit(-1); }
          write_disk(buf, i);
        }
        close(f);
        if (size & 4095) write_disk(zeros, 4096 - (size & 4095));
      }
    }
  }
  
  // update directory
  lseek(disk, dseek, SEEK_SET);
  write_disk(de, dsize);
  lseek(disk, 0, SEEK_END);
}

int main(int argc, char *argv[])
{
  struct direct *sp;
  static char cwd[PATH_MAX];
  if (sizeof(struct dinode) != 4096) { dprintf(2, "sizeof(struct dinode) %d != 4096\n", sizeof(struct dinode)); return -1; }
  
  if (argc != 3) { dprintf(2, "Usage: mkfs fs rootdir\n"); return -1; }
  if ((disk = open(argv[1], O_RDWR | O_CREAT | O_TRUNC)) < 0) { dprintf(2, "open(%s) failed\n", argv[1]); return -1; }
  if ((int)(sp = (struct direct *) sbrk(16*1024*1024)) == -1) { dprintf(2, "sbrk() failed\n"); return -1; }

  // write zero bitmap
  write_disk(buf, BUFSZ);
  
  // populate file system
  getcwd(cwd, sizeof(cwd));
  chdir(argv[2]);
  add_dir(bn = 16, sp);
  chdir(cwd);

  // update bitmap
  memset(buf, 0xff, bn / 8);
  if (bn & 7) buf[bn / 8] = (1 << (bn & 7)) - 1;
  lseek(disk, 0, SEEK_SET);
  write_disk(buf, (bn + 7) / 8);
  close(disk);
  return 0;
}
