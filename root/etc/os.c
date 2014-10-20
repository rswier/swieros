// os.c - based on xv6 with heavy modifications
#include <u.h>

enum {
  PAGE    = 4096,       // page size
  NPROC   = 64,         // maximum number of processes
  NOFILE  = 16,         // open files per process
  NFILE   = 100,        // open files per system
  NBUF    = 10,         // size of disk block cache
  NINODE  = 50,         // maximum number of active i-nodes  XXX make this more dynamic ... 
  NDEV    = 10,         // maximum major device number
  USERTOP = 0xc0000000, // end of user address space
  P2V     = +USERTOP,   // turn a physical address into a virtual address
  V2P     = -USERTOP,   // turn a virtual address into a physical address
  FSSIZE  = PAGE*1024,  // XXX
  MAXARG  = 256,        // max exec arguments
  STACKSZ = 0x800000,   // user stack size (8MB)
};

enum { // page table entry flags   XXX refactor vs. i386
  PTE_P = 0x001, // present
  PTE_W = 0x002, // writeable
  PTE_U = 0x004, // user
  PTE_A = 0x020, // accessed
  PTE_D = 0x040, // dirty
};

enum { // processor fault codes
  FMEM,   // bad physical address
  FTIMER, // timer interrupt
  FKEYBD, // keyboard interrupt
  FPRIV,  // privileged instruction
  FINST,  // illegal instruction
  FSYS,   // software trap
  FARITH, // arithmetic trap
  FIPAGE, // page fault on opcode fetch
  FWPAGE, // page fault on write
  FRPAGE, // page fault on read
  USER=16 // user mode exception
};

struct trapframe { // layout of the trap frame built on the stack by trap handler
  int sp, pad1;
  double g, f;
  int c,  pad2;
  int b,  pad3;
  int a,  pad4;
  int fc, pad5;
  int pc, pad6;
};

struct buf {
  int flags;
  uint sector;
  struct buf *prev;      // LRU cache list
  struct buf *next;
//  struct buf *qnext;     // disk queue XXX
  uchar *data;
};
enum { B_BUSY  = 1,      // buffer is locked by some process
       B_VALID = 2,      // buffer has been read from disk
       B_DIRTY = 4};     // buffer needs to be written to disk
enum { S_IFIFO = 0x1000, // fifo
       S_IFCHR = 0x2000, // character
       S_IFBLK = 0x3000, // block
       S_IFDIR = 0x4000, // directory
       S_IFREG = 0x8000, // regular
       S_IFMT  = 0xF000 }; // file type mask
enum { O_RDONLY, O_WRONLY, O_RDWR, O_CREAT = 0x100, O_TRUNC = 0x200 };
enum { SEEK_SET, SEEK_CUR, SEEK_END };

struct stat {
  ushort st_dev;         // device number
  ushort st_mode;        // type of file
  uint   st_ino;         // inode number on device
  uint   st_nlink;       // number of links to file
  uint   st_size;        // size of file in bytes
};

// disk file system format
enum {
  ROOTINO  = 16,         // root i-number
  NDIR     = 480,
  NIDIR    = 512,
  NIIDIR   = 8,
  NIIIDIR  = 4,
  DIRSIZ   = 252,
  PIPESIZE = 4000,       // XXX up to a page (since pipe is a page)
};

struct dinode { // on-disk inode structure
  ushort mode;           // file mode
  uint nlink;            // number of links to inode in file system
  uint size;             // size of file
  uint pad[17];
  uint dir[NDIR];        // data block addresses
  uint idir[NIDIR];
  uint iidir[NIIDIR];    // XXX not implemented
  uint iiidir[NIIIDIR];  // XXX not implemented
};

struct direct { // directory is a file containing a sequence of direct structures.
  uint d_ino;
  char d_name[DIRSIZ];
};

struct pipe {
  char data[PIPESIZE];
  uint nread;            // number of bytes read
  uint nwrite;           // number of bytes written
  int readopen;          // read fd is still open
  int writeopen;         // write fd is still open
};

struct inode { // in-memory copy of an inode
  uint inum;             // inode number
  int ref;               // reference count
  int flags;             // I_BUSY, I_VALID
  ushort mode;           // copy of disk inode
  uint nlink;
  uint size;
  uint dir[NDIR];
  uint idir[NIDIR];
};

enum { FD_NONE, FD_PIPE, FD_INODE, FD_SOCKET, FD_RFS };
struct file {
  int type;
  int ref;
  char readable;
  char writable;
  struct pipe *pipe;     // XXX make vnode
  struct inode *ip;
  uint off;
};

enum { I_BUSY = 1, I_VALID = 2 };
enum { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct proc { // per-process state
  struct proc *next;
  struct proc *prev;
  uint sz;               // size of process memory (bytes)
  uint *pdir;            // page directory
  char *kstack;          // bottom of kernel stack for this process
  int state;             // process state
  int pid;               // process ID
  struct proc *parent;   // parent process
  struct trapframe *tf;  // trap frame for current syscall
  int context;           // swtch() here to run process
  void *chan;            // if non-zero, sleeping on chan
  int killed;            // if non-zero, have been killed
  struct file *ofile[NOFILE]; // open files
  struct inode *cwd;     // current directory
  char name[16];         // process name (debugging)
};

struct devsw { // device implementations XXX redesign
  int (*read)();
  int (*write)();
};

enum { CONSOLE = 1 }; // XXX ditch..

enum { INPUT_BUF = 128 };
struct input_s {
  char buf[INPUT_BUF];
  uint r;  // read index
  uint w;  // write index
};

enum { PF_INET = 2, AF_INET = 2, SOCK_STREAM = 1, INADDR_ANY = 0 }; // XXX keep or chuck these?

// *** Globals ***

struct proc proc[NPROC];
struct proc *u;          // current process
struct proc *init;
char *mem_free;          // memory free list
char *mem_top;           // current top of unused memory
uint mem_sz;             // size of physical memory
uint kreserved;          // start of kernel reserved memory heap
struct devsw devsw[NDEV];
uint *kpdir;             // kernel page directory
uint ticks;
char *memdisk;
struct input_s input;    // XXX do this some other way?
struct buf bcache[NBUF];
struct buf bfreelist;    // linked list of all buffers, through prev/next.   bfreelist.next is most recently used
struct inode inode[NINODE]; // inode cache XXX make dynamic and eventually power of 2, look into iget()
struct file file[NFILE];
int nextpid;

rfsd = -1; // XXX will be set on mount, XXX total redesign?

// *** Code ***

void *memcpy(void *d, void *s, uint n) { asm(LL,8); asm(LBL, 16); asm(LCL,24); asm(MCPY); asm(LL,8); }
void *memset(void *d, uint c,  uint n) { asm(LL,8); asm(LBLB,16); asm(LCL,24); asm(MSET); asm(LL,8); }
void *memchr(void *s, uint c,  uint n) { asm(LL,8); asm(LBLB,16); asm(LCL,24); asm(MCHR); }

int in(port)    { asm(LL,8); asm(BIN); }
out(port, val)  { asm(LL,8); asm(LBL,16); asm(BOUT); }
ivec(void *isr) { asm(LL,8); asm(IVEC); }
lvadr()         { asm(LVAD); }
uint msiz()     { asm(MSIZ); }
stmr(val)       { asm(LL,8); asm(TIME); }
pdir(val)       { asm(LL,8); asm(PDIR); }
spage(val)      { asm(LL,8); asm(SPAG); }
splhi()         { asm(CLI); }
splx(int e)     { if (e) asm(STI); }

int strlen(void *s) { return memchr(s, 0, -1) - s; }

xstrncpy(char *s, char *t, int n) // no return value unlike strncpy XXX remove me only called once
{
  while (n-- > 0 && (*s++ = *t++));
  while (n-- > 0) *s++ = 0;
}

safestrcpy(char *s, char *t, int n) // like strncpy but guaranteed to null-terminate.
{
  if (n <= 0) return;
  while (--n > 0 && (*s++ = *t++));
  *s = 0;
}

// page allocator
char *kalloc()
{
  char *r; int e = splhi();
  if (r = mem_free) mem_free = *(char **)r;
  else if ((uint)(r = mem_top) < P2V+(mem_sz - FSSIZE)) mem_top += PAGE; //XXX uint issue is going to be a problem with other pointer compares!
  else panic("kalloc failure!");  //XXX need to sleep here!
  splx(e);
  return r;
}

kfree(char *v)
{
  int e = splhi();
  if ((uint)v % PAGE || v < (char *)(P2V+kreserved) || (uint)v >= P2V+(mem_sz - FSSIZE)) panic("kfree");
  *(char **)v = mem_free;
  mem_free = v;
  splx(e);
}

// console device
cout(char c)
{
  out(1, c);
}
printn(int n)
{
  if (n > 9) { printn(n / 10); n %= 10; }
  cout(n + '0');
}
printx(uint n)
{
  if (n > 15) { printx(n >> 4); n &= 15; }
  cout(n + (n > 9 ? 'a' - 10 : '0'));
}
printf(char *f, ...) // XXX simplify or chuck
{
  int n, e = splhi(); char *s; va_list v;
  va_start(v, f);
  while (*f) {
    if (*f != '%') { cout(*f++); continue; }
    switch (*++f) {
    case 'd': f++; if ((n = va_arg(v,int)) < 0) { cout('-'); printn(-n); } else printn(n); continue;
    case 'x': f++; printx(va_arg(v,int)); continue;
    case 's': f++; for (s = va_arg(v, char *); *s; s++) cout(*s); continue;
    }
    cout('%');
  }
  splx(e);
}

panic(char *s)
{
  asm(CLI);
  out(1,'p'); out(1,'a'); out(1,'n'); out(1,'i'); out(1,'c'); out(1,':'); out(1,' '); 
  while (*s) out(1,*s++);
  out(1,'\n');
  asm(HALT);
}

consoleintr()
{
  int c;
  while ((c = in(0)) != -1) {
//    printf("<%d>",c); //   XXX
    if (input.w - input.r < INPUT_BUF) {
      input.buf[input.w++ % INPUT_BUF] = c;
      wakeup(&input.r);
    }
  }
}

int consoleread(struct inode *ip, char *dst, int n)
{
  int target, c, e;

  iunlock(ip);
  target = n;
  e = splhi();
  while (n > 0) {
    if (input.r == input.w && n < target) break; // block until at least one byte transfered
    while (input.r == input.w) {
      if (u->killed) {
        splx(e);
        ilock(ip);
        return -1;
      }
      sleep(&input.r);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    *dst++ = c;  // XXX pagefault possible in cli (perhaps use inode locks to achieve desired effect)
    n--;
  }
  splx(e);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{
  int i, e;

  iunlock(ip);
  e = splhi(); // XXX pagefault possible in cli
  for (i = 0; i < n; i++) cout(buf[i]);
  splx(e);
  ilock(ip);
  return n;
}

consoleinit()
{
  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read  = consoleread;
}

// fake IDE disk; stores blocks in memory.  useful for running kernel without scratch disk.  XXX but no good for stressing demand pageing logic!
ideinit()
{
  memdisk = P2V+(mem_sz - FSSIZE);
}

// sync buf with disk.  if B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// else if B_VALID is not set, read buf from disk, set B_VALID.
iderw(struct buf *b) // XXX rename?!
{
  if (!(b->flags & B_BUSY)) panic("iderw: buf not busy");
  if ((b->flags & (B_VALID|B_DIRTY)) == B_VALID) panic("iderw: nothing to do");
  if (b->sector >= (FSSIZE / PAGE)) panic("iderw: sector out of range");

  if (b->flags & B_DIRTY) {
    b->flags &= ~B_DIRTY;
    memcpy(memdisk + b->sector*PAGE, b->data, PAGE);
  } else
    memcpy(b->data, memdisk + b->sector*PAGE, PAGE);
  b->flags |= B_VALID;
}

// buffer cache:
// The buffer cache is a linked list of buf structures holding cached copies of disk block contents.  Caching disk blocks.
// in memory reduces the number of disk reads and also provides a synchronization point for disk blocks used by multiple processes.
// 
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer, so do not keep them longer than necessary.
// 
// The implementation uses three state flags internally:
// * B_BUSY: the block has been returned from bread and has not been passed back to brelse.  
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified and needs to be written to disk.
binit()
{
  struct buf *b;

  // create linked list of buffers
  bfreelist.prev = bfreelist.next = &bfreelist;
  for (b = bcache; b < bcache+NBUF; b++) {
    b->next = bfreelist.next;
    b->prev = &bfreelist;
    b->data = kalloc();
    bfreelist.next->prev = b;
    bfreelist.next = b;
  }
}

// look through buffer cache for sector.
// if not found, allocate fresh block.  in either case, return B_BUSY buffer
struct buf *bget(uint sector)
{
  struct buf *b; int e = splhi();
  
loop:  // try for cached block
  for (b = bfreelist.next; b != &bfreelist; b = b->next) {
    if (b->sector == sector) {
      if (!(b->flags & B_BUSY)) {
        b->flags |= B_BUSY;
        splx(e);
        return b;
      }
      sleep(b);
      goto loop;
    }
  }

  // allocate fresh block
  for (b = bfreelist.prev; b != &bfreelist; b = b->prev) {
    if (!(b->flags & (B_BUSY | B_DIRTY))) {
      b->sector = sector;
      b->flags = B_BUSY;
      splx(e);
      return b;
    }
  }
  panic("bget: no buffers");
}

// return a B_BUSY buf with the contents of the indicated disk sector
struct buf *bread(uint sector)
{
  struct buf *b;

  b = bget(sector);
  if (!(b->flags & B_VALID)) iderw(b);
  return b;
}

// write b's contents to disk.  must be B_BUSY
bwrite(struct buf *b)
{
  if (!(b->flags & B_BUSY)) panic("bwrite");
  b->flags |= B_DIRTY;
  iderw(b);
}

// release a B_BUSY buffer.  move to the head of the MRU list
brelse(struct buf *b)
{
  int e = splhi();
  if (!(b->flags & B_BUSY)) panic("brelse");

  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bfreelist.next;
  b->prev = &bfreelist;
  bfreelist.next->prev = b;
  bfreelist.next = b;
  b->flags &= ~B_BUSY;
  wakeup(b);
  splx(e);
}

// file system implementation.  four layers:
//   Blocks      - allocator for disk blocks.
//   Files       - inode allocator, reading, writing, metadata.
//   Directories - inode with special contents (list of other inodes!)
//   Names

// zero a block
bzero(uint b)  // XXX only called in bfree
{
  struct buf *bp;
  bp = bread(b);
  memset(bp->data, 0, PAGE);
  bwrite(bp);
  brelse(bp);
}

// allocate a disk block
uint balloc()
{
  int b, bi, bb;
  struct buf *bp;

  for (b = 0; b < 16; b++) {
    bp = bread(b);
    for (bi = 0; bi < 4096; bi++) {
      if (bp->data[bi] == 0xff) continue;
      for (bb = 0; bb < 8; bb++) {
        if (bp->data[bi] & (1 << bb)) continue; // is block free?
        bp->data[bi] |= (1 << bb);  // mark block in use
        bwrite(bp);
        brelse(bp);
        return b*(4096*8) + bi*8 + bb;
      }
    }
    brelse(bp);
  }
  panic("balloc: out of blocks");
}

// free a disk block
bfree(uint b)
{
  int bi, m;
  struct buf *bp;

  bzero(b);

  bp = bread(b / (4096*8));
  m = 1 << (b & 7);
  b = (b / 8) & 4095;
  if (!(bp->data[b] & m)) panic("freeing free block");
  bp->data[b] &= ~m;  // mark block free on disk
  bwrite(bp);
  brelse(bp);
}

// Inodes:
// An inode is a single, unnamed file in the file system.  The inode disk structure holds metadata
// (the type, device numbers, and data size) along with a list of blocks where the data can be found.
//
// The kernel keeps a cache of the in-use on-disk structures to provide a place for synchronizing access
// to inodes shared between multiple processes.
// 
// ip->ref counts the number of pointer references to this cached inode; references are typically kept in
// struct file and in u->cwd.  When ip->ref falls to zero, the inode is no longer cached.  It is an error
// to use an inode without holding a reference to it.
//
// Processes are only allowed to read and write inode metadata and contents when holding the inode's lock,
// represented by the I_BUSY flag in the in-memory copy.  Because inode locks are held during disk accesses, 
// they are implemented using a flag rather than with spin locks.  Callers are responsible for locking
// inodes before passing them to routines in this file; leaving this responsibility with the caller makes
// it possible for them to create arbitrarily-sized atomic operations.
//
// To give maximum control over locking to the callers, the routines in this file that return inode pointers 
// return pointers to *unlocked* inodes.  It is the callers' responsibility to lock them before using them.
// A non-zero ip->ref keeps these unlocked inodes in the cache.

// find the inode with number inum and return the in-memory copy.  does not lock the inode and does not read it from disk
struct inode *iget(uint inum)
{
  struct inode *ip, *empty; int e = splhi();

  // is the inode already cached
  empty = 0;
  for (ip = &inode[0]; ip < &inode[NINODE]; ip++) {
    if (ip->ref > 0 && ip->inum == inum) {
      ip->ref++;
      splx(e);
      return ip;
    }
    if (!empty && !ip->ref) empty = ip; // remember empty slot
  }

  // recycle an inode cache entry
  if (!empty) panic("iget: no inodes");

  ip = empty;
  ip->inum = inum;
  ip->ref = 1;
  ip->flags = 0;
  splx(e);
  
  return ip;
}

// allocate a new inode with the given mode
struct inode *ialloc(ushort mode)
{
  int inum;
  struct buf *bp;
  struct dinode *dip;

  inum = balloc();
  bp = bread(inum);
  dip = (struct dinode *)bp->data;
  memset(dip, 0, sizeof(*dip));
  dip->mode = mode;
  bwrite(bp);   // mark it allocated on the disk
  brelse(bp);
  return iget(inum);
}

// copy modified memory inode to disk
iupdate(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;

  bp = bread(ip->inum);
  dip = (struct dinode *)bp->data;
  dip->mode  = ip->mode;
  dip->nlink = ip->nlink;
  dip->size = ip->size;
//  printf("iupdate() memcpy(dip->dir, ip->dir, %d)\n",sizeof(ip->dir));
  memcpy(dip->dir, ip->dir, sizeof(ip->dir));
  memcpy(dip->idir, ip->idir, sizeof(ip->idir));
  bwrite(bp);
  brelse(bp);
}

// increment reference count for ip
idup(struct inode *ip)
{
  int e = splhi();
  ip->ref++;
  splx(e);
}

// lock the given inode.  read the inode from disk if necessary
ilock(struct inode *ip)
{
  struct buf *bp;
  struct dinode *dip;
  int e = splhi();

  if (!ip || ip->ref < 1) panic("ilock");

  while (ip->flags & I_BUSY) sleep(ip);
  ip->flags |= I_BUSY;
  splx(e);
  
  if (!(ip->flags & I_VALID)) {
    bp = bread(ip->inum);
    dip = (struct dinode *)bp->data;
    ip->mode  = dip->mode;
    ip->nlink = dip->nlink;
    ip->size = dip->size;
    memcpy(ip->dir,  dip->dir,  sizeof(ip->dir));
    memcpy(ip->idir, dip->idir, sizeof(ip->idir));
    brelse(bp);
    ip->flags |= I_VALID;
    if (!ip->mode) panic("ilock: no mode");
  }
}

// unlock the given inode
iunlock(struct inode *ip)
{
  int e = splhi();
  if (!ip || !(ip->flags & I_BUSY) || ip->ref < 1) panic("iunlock");

  ip->flags &= ~I_BUSY;
  wakeup(ip);
  splx(e);
}

// drop a reference to an in-memory inode
// if that was the last reference, the inode cache entry can be recycled
// if that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk
iput(struct inode *ip)
{
  int e = splhi();
  if (ip->ref == 1 && (ip->flags & I_VALID) && !ip->nlink) {
    // inode has no links: truncate and free inode
    if (ip->flags & I_BUSY) panic("iput busy");
    ip->flags |= I_BUSY;
    splx(e);
    itrunc(ip);
    ip->mode = 0;
    bfree(ip->inum); 
    e = splhi();
    ip->flags = 0;
    wakeup(ip);
  }
  ip->ref--;
  splx(e);
}

// common idiom: unlock, then put
iunlockput(struct inode *ip)
{
  iunlock(ip);
  iput(ip);
}

// Inode contents:
// The contents (data) associated with each inode is stored in a sequence of blocks on the disk.
// The first NDIR blocks are listed in ip->dir[].  The next NIDIR blocks are listed in the block ip->idir[].
// Return the disk block address of the nth block in inode ip. If there is no such block, bmap allocates one.
uint bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;

  if (bn < NDIR) {
    if (!(addr = ip->dir[bn])) ip->dir[bn] = addr = balloc();
    return addr;
  }
  bn -= NDIR;
  if (bn >= NIDIR * 1024) panic("bmap: out of range");

  // load indirect block, allocating if necessary
  if (!(addr = ip->idir[bn / 1024])) ip->idir[bn / 1024] = addr = balloc();
  bp = bread(addr);
  a = (uint *)bp->data;
  if (!(addr = a[bn & 1023])) {
    a[bn & 1023] = addr = balloc();
    bwrite(bp);
  }
  brelse(bp);
  return addr;
}

// truncate inode (discard contents)
// only called when the inode has no links to it (no directory entries referring to it)
// and has no in-memory reference to it (is not an open file or current directory)
itrunc(struct inode *ip)
{
  int i, j;
  struct buf *bp;
  uint *a;

  for (i = 0; i < NDIR; i++) {
    if (!ip->dir[i]) goto done;  // XXX done by ip->size?
    bfree(ip->dir[i]);
    ip->dir[i] = 0;
  }
  
  for (i = 0; i < NIDIR; i++) {
    if (!ip->idir[i]) break;
    bp = bread(ip->idir[i]);
    a = (uint *)bp->data;
    for (j = 0; j < 1024; j++) {
      if (!a[j]) break;
      bfree(a[j]);
    }
    brelse(bp);
    bfree(ip->idir[i]);
    ip->idir[i] = 0;
  }

done:
  ip->size = 0;
  iupdate(ip);
}

// copy stat information from inode
stati(struct inode *ip, struct stat *st)
{
  st->st_dev   = 0; // XXX
  st->st_mode  = ip->mode;
  st->st_ino   = ip->inum;
  st->st_nlink = ip->nlink;
  st->st_size  = ip->size;
}

// read data from inode
int readi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if ((ip->mode & S_IFMT) == S_IFCHR) { // S_IFBLK ??
//    if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read) return -1;
//    return devsw[ip->major].read(ip, dst, n);
    if (ip->dir[0] >= NDEV || !devsw[ip->dir[0]].read) return -1; // XXX refactor
    return devsw[ip->dir[0]].read(ip, dst, n);
  }

  if (off > ip->size || off + n < off) return -1;
  if (off + n > ip->size) n = ip->size - off;

  for (tot = n; tot; tot -= m, off += m, dst += m) {
    bp = bread(bmap(ip, off/PAGE));
    if ((m = PAGE - off%PAGE) > tot) m = tot;
    memcpy(dst, bp->data + off%PAGE, m);
    brelse(bp);
  }
  return n;
}

// write data to inode
int writei(struct inode *ip, char *src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if ((ip->mode & S_IFMT) == S_IFCHR) { // XXX S_IFBLK ??
//    if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write) return -1;
//    return devsw[ip->major].write(ip, src, n);
    if (ip->dir[0] >= NDEV || !devsw[ip->dir[0]].write) return -1; // XXX refactor
    return devsw[ip->dir[0]].write(ip, src, n);
  }
  if (off > ip->size || off + n < off) return -1;
  if (off + n > (NDIR + NIDIR*1024)*PAGE) return -1;

  for (tot = n; tot; tot -= m, off += m, src += m) {
    bp = bread(bmap(ip, off/PAGE));
    if ((m = PAGE - off%PAGE) > tot) m = tot;
    memcpy(bp->data + off%PAGE, src, m);
    bwrite(bp);
    brelse(bp);
  }
  if (n > 0 && off > ip->size) {
    ip->size = off;
    iupdate(ip);
  }
  return n;
}

// directories:
int namecmp(char *p, char *q) 
{
  uint n = DIRSIZ;
  while (n) { if (!*p || *p != *q) return *p - *q; n--; p++; q++; } // XXX
  return 0;
}

// look for a directory entry in a directory. If found, set *poff to byte offset of entry.
struct inode *dirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off; struct direct de;

  if ((dp->mode & S_IFMT) != S_IFDIR) panic("dirlookup not DIR");
  for (off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) panic("dirlink read");
    if (de.d_ino && !namecmp(name, de.d_name)) { // entry matches path element
      if (poff) *poff = off;
      return iget(de.d_ino);
    }
  }
  return 0;
}

// write a new directory entry (name, inum) into the directory dp
int dirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct direct de;
  struct inode *ip;

  // check that name is not present
  if (ip = dirlookup(dp, name, 0)) {
    iput(ip);
    return -1;
  }
  // look for an empty direct
  for (off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) panic("dirlink read");
    if (!de.d_ino) break;
  }
  xstrncpy(de.d_name, name, DIRSIZ);
  de.d_ino = inum;
  if (writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) panic("dirlink");
  
  return 0;
}

// is the directory dp empty except for "." and ".." ?
int isdirempty(struct inode *dp)
{
  int off;
  struct direct de;

  for (off=2*sizeof(de); off<dp->size; off+=sizeof(de)) {
    if (readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) panic("isdirempty: readi");
    if (de.d_ino) return 0;
  }
  return 1;
}

// paths:
// Copy the next path element from path into name.  Return a pointer to the element following the copied one.
// The returned path has no leading slashes, so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
char *skipelem(char *path, char *name)
{
  char *s;
  int len;

  while (*path == '/') path++;
  if (!*path) return 0;
  s = path;
  while (*path != '/' && *path) path++;
  len = path - s;
  if (len >= DIRSIZ) memcpy(name, s, DIRSIZ);
  else {
    memcpy(name, s, len);
    name[len] = 0;
  }
  while (*path == '/') path++;
  return path;
}

// Look up and return the inode for a path name
struct inode *namei(char *path)
{
  char name[DIRSIZ];
  struct inode *ip, *next;

  if (*path == '/') ip = iget(ROOTINO); else idup(ip = u->cwd);
  
  while (path = skipelem(path, name)) {
    ilock(ip);
    if ((ip->mode & S_IFMT) != S_IFDIR || !(next = dirlookup(ip, name, 0))) {
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  return ip;
}
// return the inode for the parent and copy the final path element into name, which must have room for DIRSIZ bytes.
struct inode *nameiparent(char *path, char *name)
{
  struct inode *ip, *next;

  if (*path == '/') ip = iget(ROOTINO); else idup(ip = u->cwd);
  
  while (path = skipelem(path, name)) {
    ilock(ip);
    if ((ip->mode & S_IFMT) != S_IFDIR) {
      iunlockput(ip);
      return 0;
    }
    if (!*path) { // stop one level early
      iunlock(ip);
      return ip;
    }
    if (!(next = dirlookup(ip, name, 0))) {
      iunlockput(ip);
      return 0;
    }
    iunlockput(ip);
    ip = next;
  }
  iput(ip);
  return 0;
}

// pipes:
void pipeclose(struct pipe *p, int writable)
{
  int e = splhi();
  if (writable) {
    p->writeopen = 0;
    wakeup(&p->nread);
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if (!p->readopen && !p->writeopen) kfree(p);
  splx(e);
}

int pipewrite(struct pipe *p, char *addr, int n)
{
  int i, e = splhi();

  for (i = 0; i < n; i++) {
    while (p->nwrite == p->nread + PIPESIZE) {  // XXX DOC: pipewrite-full
      if (!p->readopen || u->killed) {
        splx(e);
        return -1;
      }
      wakeup(&p->nread);
      sleep(&p->nwrite);  // XXX DOC: pipewrite-sleep
    }
    p->data[p->nwrite++ % PIPESIZE] = addr[i]; // XXX pagefault possible in cli (use inode locks instead?)
  }
  wakeup(&p->nread);  // XXX DOC: pipewrite-wakeup
  splx(e);
  return n;
}

int piperead(struct pipe *p, char *addr, int n)
{
  int i, e = splhi();

  while (p->nread == p->nwrite && p->writeopen) {  // XXX DOC: pipe-empty
    if (u->killed) {
      splx(e);
      return -1;
    }
    sleep(&p->nread); // XXX DOC: piperead-sleep
  }
  for (i = 0; i < n; i++) {  // XXX DOC: piperead-copy
    if (p->nread == p->nwrite) break;
    addr[i] = p->data[p->nread++ % PIPESIZE]; // XXX pagefault possible in cli (use inode locks instead?)
  }
  wakeup(&p->nwrite);  // XXX DOC: piperead-wakeup
  splx(e);
  return i;
}

// allocate a file structure
struct file *filealloc()
{
  struct file *f; int e = splhi();

  for (f = file; f < file + NFILE; f++) {
    if (!f->ref) {
      f->ref = 1;
      splx(e);
      return f;
    }
  }
  splx(e);
  return 0;
}

// increment ref count for file
struct file *filedup(struct file *f)
{
  int e = splhi();
  if (f->ref < 1) panic("filedup");
  f->ref++;
  splx(e);
  return f;
}

// allocate a file descriptor for the given file.  Takes over file reference from caller on success.
int fdalloc(struct file *f)
{
  int fd;

  for (fd = 0; fd < NOFILE; fd++) {
    if (!u->ofile[fd]) {
      u->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

struct inode *create(char *path, ushort mode, int dev)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if (!(dp = nameiparent(path, name))) return 0;
  ilock(dp);

  if (ip = dirlookup(dp, name, 0)) {
    iunlockput(dp);
    ilock(ip);
    if ((mode & S_IFMT) == S_IFREG && (ip->mode & S_IFMT) == S_IFREG) return ip;
    iunlockput(ip);
    return 0;
  }

  if (!(ip = ialloc(mode))) panic("create: ialloc");

  ilock(ip);
  if ((mode & S_IFMT) == S_IFCHR || (ip->mode & S_IFMT) == S_IFBLK) {
    ip->dir[0] = (dev >> 8) & 0xff;
    ip->dir[1] = dev & 0xff;
  }
  ip->nlink = 1;
  iupdate(ip);

  if ((mode & S_IFMT) == S_IFDIR) {  // create . and .. entries
    dp->nlink++;  // for ".."
    iupdate(dp);
    // no ip->nlink++ for ".": avoid cyclic ref count
    if (dirlink(ip, ".", ip->inum) || dirlink(ip, "..", dp->inum)) panic("create dots");
  }

  if (dirlink(dp, name, ip->inum)) panic("create: dirlink");

  iunlockput(dp);
  return ip;
}

// *** syscalls ***
int svalid(uint s) { return (s < u->sz) && memchr(s, 0, u->sz - s); }
int mvalid(uint a, int n) { return a <= u->sz && a+n <= u->sz; }
struct file *getf(uint fd) { return (fd < NOFILE) ? u->ofile[fd] : 0; }

int sockopen(int family, int type, int protocol) { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(NET1); }  
sockclose(int sd) { asm(LL,8); asm(NET2); } // XXX
int sockconnect(int fd, uint family_port, uint addr) { asm(LL, 8); asm(LBL,16); asm(LCL,24); asm(NET3); }
int sockread (int sd, char *addr, int n) { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(NET4); }
int sockwrite(int sd, char *addr, int n) { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(NET5); }
int sockpoll(int sd) { asm(LL, 8); asm(NET6); }
enum { M_OPEN, M_CLOSE, M_READ, M_WRITE, M_SEEK, M_FSTAT, M_SYNC };

int socktx(int sd, void *p, int n)
{
  int r;
  while (n > 0) {
    if ((r = sockwrite(sd, p, n)) <= 0) return -1;  // XXX need some kind of ready to write? XXX <= 0?
    n -= r;
    p += r;
  }
  return 0;
}
// int cyc() { asm(CYC); } XXX
int sockrx(int sd, void *p, int n) // XXX this is always going to be slow until I fix the sockpoll (since ssleep waits for a comparitively long time (timer vs. em delta))
{
  int r; // uint cy, nyc;
  while (n > 0) {
    while (!sockpoll(sd)) ssleep(1); // XXX needs to block properly XXX should I lock the inode?
//    while (!sockpoll(sd)) { printf("x"); ssleep(1); } // XXX needs to block properly XXX should I lock the inode?
//    while (!sockpoll(sd)) { cy = cyc(); for (r = 0; r < 10; r++) { nyc = cyc(); printf("[%d]",nyc - cy); cy = nyc; ssleep(100); } } // XXX needs to block properly XXX should I lock the inode?
//    while (!sockpoll(sd)) { cy = cyc(); for (r = 0; r < 10; r++) { nyc = cyc(); printf("[%d]",nyc - cy); cy = nyc; ssleep(1); } } // XXX needs to block properly XXX should I lock the inode?
    if ((r = sockread(sd, p, n)) <= 0) { printf("sockrx() sockread()\n"); return -1; } //  XXX <= 0?
    n -= r;
    p += r; 
  }
  return 0;
}

fileclose(struct file *f)
{
  struct file ff; int e = splhi();

  if (f->ref < 1) panic("close");
  if (--f->ref > 0) {
    splx(e);
    return;
  }
  memcpy(&ff, f, sizeof(struct file)); //XXX  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  splx(e);
  
  switch (ff.type) {
  case FD_PIPE:   pipeclose(ff.pipe, ff.writable); break;
  case FD_INODE:  iput(ff.ip); break;
  case FD_SOCKET:
  case FD_RFS:    sockclose(ff.off);
  }
}

int close(int fd)
{
  struct file *f;
  if (!(f = getf(fd))) return -1;
  u->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int fstat(int fd, struct stat *st)
{
  int r; struct file *f;
  if (!(f = getf(fd)) || !mvalid(st, sizeof(struct stat))) return -1;
  switch (f->type) {
  case FD_INODE:
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
 case FD_RFS:
    r = M_FSTAT;
    if (socktx(f->off, &r, 4) || sockrx(f->off, &r, 4) || (r == 0 && sockrx(f->off, st, sizeof(struct stat)))) return -1;
    return r;
  }
  return -1;
}

int read(int fd, char *addr, int n)
{
  int r; int h[2]; struct file *f;
  if (!(f = getf(fd)) || !f->readable || !mvalid(addr, n)) return -1;
  switch (f->type) {
  case FD_PIPE: return piperead(f->pipe, addr, n);
  case FD_SOCKET:
    while (!sockpoll(f->off)) ssleep(1); // XXX needs to block properly XXX should I lock the inode? (right now there isn't an inode!)
    return sockread(f->off, addr, n);
  case FD_INODE:
    ilock(f->ip);
    if ((r = readi(f->ip, addr, f->off, n)) > 0) f->off += r;
    iunlock(f->ip);
    return r;
  case FD_RFS:
    h[0] = M_READ; // XXX lock the inode
    h[1] = n;
    if (socktx(f->off, h, 8) || sockrx(f->off, &r, 4) || ((uint)r <= n && sockrx(f->off, addr, r))) return -1; // XXX need to zortch the file  is this correct for zero?
    return r;
  }
  panic("read");
}
int write(int fd, char *addr, int n)
{
  int r, h[2]; struct file *f;
  if (!(f = getf(fd)) || !f->writable || !mvalid(addr, n)) return -1;
  switch (f->type) {
  case FD_PIPE: return pipewrite(f->pipe, addr, n);
  case FD_SOCKET: return sockwrite(f->off, addr, n); // XXX needs to block
  case FD_INODE:
    ilock(f->ip);
    if ((r = writei(f->ip, addr, f->off, n)) > 0) f->off += r;
    iunlock(f->ip);
    return r;
  case FD_RFS:
    h[0] = M_WRITE; // XXX lock the inode
    h[1] = n;
    if (socktx(f->off, h, 8) || (n > 0 && socktx(f->off, addr, n))) return -1; // XXX need to zortch the file
    return n;
  }
  panic("write");
}

int lseek(int fd, int offset, uint whence)
{
  int r, h[3]; struct file *f;
  if (!(f = getf(fd)) || whence > SEEK_END) return -1;
  switch (f->type) {
  case FD_INODE:
    ilock(f->ip);
    switch (whence) {
    case SEEK_SET: r = f->off = offset; break;
    case SEEK_CUR: r = f->off += offset; break;
    case SEEK_END: r = f->off = f->ip->size - offset; break; // XXX verify
    }
    iunlock(f->ip);
    return r;
  case FD_RFS:
    h[0] = M_SEEK;
    h[1] = offset;
    h[2] = whence;
    if (socktx(f->off, h, 12) || sockrx(f->off, &r, 4)) return -1;
    return r;
  }
  return -1;
}

int dup2(int fd, int d)
{
  struct file *f;
  if (!(f = getf(fd)) || (uint)d >= NOFILE) return -1;
  close(d);
  u->ofile[d] = filedup(f);
  return d;
}

int link(char *old, char *new)
{
  char name[DIRSIZ];
  struct inode *dp, *ip;

  if (!svalid(old) || !svalid(new) || !(ip = namei(old))) return -1;
  ilock(ip);
  if ((ip->mode & S_IFMT) == S_IFDIR) {
    iunlockput(ip);
    return -1;
  }
  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if (!(dp = nameiparent(new, name))) goto bad;
  ilock(dp);
  if (dirlink(dp, name, ip->inum)) {
    iunlockput(dp);
bad:
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    return -1;
  }
  iunlockput(dp);
  iput(ip);
  return 0;
}

int unlink(char *path)
{
  struct inode *ip, *dp;
  struct direct de;
  char name[DIRSIZ];
  uint off;
  
  if (!svalid(path) || !(dp = nameiparent(path, name))) return -1;
  ilock(dp);

  // Cannot unlink "." or "..".
  if (!namecmp(name, ".") || !namecmp(name, "..") || !(ip = dirlookup(dp, name, &off))) goto bad;
  ilock(ip);
  if (!ip->nlink) panic("unlink: nlink == 0");
  if ((ip->mode & S_IFMT) == S_IFDIR && !isdirempty(ip)) {
    iunlockput(ip);
bad:
    iunlockput(dp);
    return -1;
  }

  memset(&de, 0, sizeof(de));
  if (writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) panic("unlink: writei");
  if ((ip->mode & S_IFMT) == S_IFDIR) {
    dp->nlink--;
    iupdate(dp);
  }
  
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  return 0;
}

uint htonl(uint a) { return (a >> 24) | ((a >> 8) & 0xff00) | ((a << 8) & 0xff0000) | (a << 24); } // XXX eliminate
ushort htons(ushort a) { return (a >> 8) | (a << 8); } // XXX eliminate
int memcmp() { asm(LL,8); asm(LBL, 16); asm(LCL,24); asm(MCMP); } // XXX eliminate
int open(char *path, int oflag) // XXX, int mode)
{
  int fd, r; int h[4];
  struct file *f;
  struct inode *ip;

  if (!svalid(path)) return -1;
//  if (!namecmp(path, "rfs.txt")) {
  if (!memcmp(path,"rfs/",4)) {
    path += 4;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1; // XXX reuse?
    f = getf(fd);
    if (sockconnect(f->off, AF_INET | (htons(5003) << 16), htonl(0x7f000001))) return -1; // XXX close?
    f->type = FD_RFS;
    h[0] = M_OPEN;
    h[1] = oflag;
    h[2] = 0; //mode;
    h[3] = strlen(path) + 1;
    if (socktx(f->off, h, 16) || socktx(f->off, path, h[3]) || sockrx(f->off, &r, 4) || r < 0) return -1; // XXX sockclose?
    return fd;
  } else if (oflag & O_CREAT) {
    if (!(ip = create(path, S_IFREG, 0))) return -1;
  } else {
    if (!(ip = namei(path))) return -1;
    ilock(ip);
    if ((ip->mode & S_IFMT) == S_IFDIR && oflag != O_RDONLY) {
      iunlockput(ip);
      return -1;
    }
  }

  if (!(f = filealloc()) || (fd = fdalloc(f)) < 0) {
    if (f) fileclose(f);
    iunlockput(ip);
    return -1;
  }

  if (oflag & O_TRUNC)
    itrunc(ip);

  iunlock(ip);

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(oflag & O_WRONLY);
  f->writable = (oflag & O_WRONLY) || (oflag & O_RDWR);
  return fd;
}

int mkdir(char *path)
{
  struct inode *ip;
  if (!svalid(path) || !(ip = create(path, S_IFDIR, 0))) return -1;
  iunlockput(ip);
  return 0;
}

int mknod(char *path, int mode, int dev)
{
  struct inode *ip;    
  if (!svalid(path) || !(ip = create(path, mode, dev))) return -1;
  iunlockput(ip);
  return 0;
}

int chdir(char *path)
{
  struct inode *ip;

  if (!svalid(path) || !(ip = namei(path))) return -1;
  ilock(ip);
  if ((ip->mode & S_IFMT) != S_IFDIR) {
    iunlockput(ip);
    return -1;
  }
  iunlock(ip);
  iput(u->cwd);
  u->cwd = ip;
  return 0;
}

int pipe(int *fd)
{
  struct pipe *p;
  struct file *rf, *wf;
  int fd0, fd1;

  if (!mvalid(fd, 8) || !(rf = filealloc())) return -1;
  if (!(wf = filealloc())) { fileclose(rf); return -1; }
  p = (struct pipe *)kalloc();
  p->readopen = 1;
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  rf->type = FD_PIPE;
  rf->readable = 1;
  rf->writable = 0;
  rf->pipe = p;
  wf->type = FD_PIPE;
  wf->readable = 0;
  wf->writable = 1;
  wf->pipe = p;

  if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
    if (fd0 >= 0) u->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}
uint *walkpdir(uint *pd, uint va);

int exec(char *path, char **argv)
{
  char *s, *last;
  uint argc, sz, sp, *stack, *pd, *oldpd, *pte;
  struct { uint magic, bss, entry, flags; } hdr;
  struct inode *ip;
  char cpath[16];  // XXX length, safety!
  int i, n, c;

  if (!svalid(path)) return -1;
  for (argc = 0; ; argc++) {
    if (argc >= MAXARG || !mvalid(argv + argc, 4)) return -1;
    if (!argv[argc]) break;
    if (!svalid(argv[argc])) return -1;
  }

  if (c = !(ip = namei(path))) {  
    memcpy(cpath, path, i = strlen(path));
    cpath[i] = '.';
    cpath[i+1] = 'c';
    cpath[i+2] = 0;
    if (!namei(cpath)) return -1;
    if (!(ip = namei(path = "/bin/c"))) return -1;
    argv++;
    argc++;
  }
  ilock(ip);
  pd = 0;

  // Check header
  if (readi(ip, (char *)&hdr, 0, sizeof(hdr)) < sizeof(hdr)) goto bad;
  if (hdr.magic != 0xC0DEF00D) goto bad; // XXX some more hdr checking?

  pd = memcpy(kalloc(), kpdir, PAGE);

  // XXX stack should go after heap

  // load text and data segment   XXX map the whole file copy on write
  if (!(sz = allocuvm(pd, 0, ip->size, 1))) goto bad;

  for (i = 0; i < sz; i += PAGE) {
    if (!(pte = walkpdir(pd, i))) panic("exit() address should exist");
    n = (sz - i < PAGE) ? sz - i : PAGE;
    if (readi(ip, P2V+(*pte & -PAGE), i, n) != n) goto bad;
  }
  
  iunlockput(ip);
  ip = 0;
  
  // allocate bss and stack segment
  if (!(sz = allocuvm(pd, sz, sz + hdr.bss + STACKSZ, 0))) goto bad;

  // initialize the top page of the stack
  sz &= -PAGE;
  mappage(pd, sz, V2P+(sp = memset(kalloc(), 0, PAGE)), PTE_P | PTE_W | PTE_U);

  // prepare stack arguments
  stack = sp += PAGE - (argc+1)*4;
  for (i=0; i<argc; i++) {
    s = (!c || i > 1) ? *argv++ : (i ? cpath : path);
    n = strlen(s) + 1;
    if ((sp & (PAGE - 1)) < n) goto bad;
    sp -= n;
    memcpy(sp, s, n);
    stack[i] = sz + (sp & (PAGE - 1));
  }
  stack[argc] = 0;
  if ((sp & (PAGE - 1)) < 40) { // XXX 40? stick into above loop?
bad:
    if (pd) freevm(pd);
    if (ip) iunlockput(ip);
    return -1;
  }
  stack = sp = (sp - 28) & -8;
  stack[0] = sz + ((sp + 24) & (PAGE - 1)); // return address
  stack[2] = argc;
  stack[4] = sz + PAGE - (argc+1)*4; // argv
  stack[6] = TRAP | (S_exit<<8); // call exit if main returns

  // save program name for debugging XXX
  for (last=s=path; *s; s++)
    if (*s == '/') last = s+1;
  safestrcpy(u->name, last, sizeof(u->name));
  
  // commit to the user image
  oldpd = u->pdir;
  u->pdir = pd;
  u->sz = sz + PAGE;
  u->tf->fc = USER;
  u->tf->pc = hdr.entry + sizeof(hdr);
  u->tf->sp = sz + (sp & (PAGE - 1));
  pdir(V2P+(uint)(u->pdir));
  freevm(oldpd);
  return 0;
}

struct proc *allocproc();
uint *copyuvm(uint *pd, uint sz);

int fork()
{
  int i, pid;
  struct proc *np;

  if (!(np = allocproc())) return -1;
  np->pdir = copyuvm(u->pdir, u->sz); // copy process state
  np->sz = u->sz;
  np->parent = u;
  memcpy(np->tf, u->tf, sizeof(struct trapframe));
  np->tf->a = 0; // child returns 0
  for (i = 0; i < NOFILE; i++)
    if (u->ofile[i]) np->ofile[i] = filedup(u->ofile[i]);
  idup(np->cwd = u->cwd);
  pid = np->pid;
  safestrcpy(np->name, u->name, sizeof(u->name));
  np->state = RUNNABLE;
  return pid;
}

// Exit the current process.  Does not return.  An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.  Special treatment for process 0 and 1.
exit(int rc)
{
  struct proc *p; int fd;

//  printf("exit(%d)\n",rc); // XXX do something with return code
  if (u->pid == 0) { for (;;) asm(IDLE); } // spin in the arms of the kernel (cant be paged out)
  else if (u->pid == 1) panic("exit() init exiting"); // XXX reboot after all processes go away?

  // close all open files
  for (fd = 0; fd < NOFILE; fd++) {
    if (u->ofile[fd]) {
      fileclose(u->ofile[fd]);
      u->ofile[fd] = 0;
    }
  }
  iput(u->cwd);
  u->cwd = 0;

  asm(CLI);

  // parent might be sleeping in wait()
  wakeup(u->parent);

  // pass abandoned children to init
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->parent == u) {
      p->parent = init;
      if (p->state == ZOMBIE) wakeup(init);
    }
  }

  // jump into the scheduler, never to return
  u->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Kill the process with the given pid.  Process won't exit until it returns to user space (see trap()).
int kill(int pid)
{
  struct proc *p; int e = splhi();

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == pid) {
      p->killed = 1;
      // wake process from sleep if necessary
      if (p->state == SLEEPING) p->state = RUNNABLE;
      splx(e);
      return 0;
    }
  }
  splx(e);
  return -1;
}

// Wait for a child process to exit and return its pid.  Return -1 if this process has no children.
int wait()
{
  struct proc *p;
  int havekids, pid, e = splhi();

  for (;;) { // scan through table looking for zombie children
    havekids = 0;
    for (p = proc; p < &proc[NPROC]; p++) {
      if (p->parent != u) continue;
      havekids = 1;
      if (p->state == ZOMBIE) {
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        splx(e);
        return pid;
      }
    }

    // no point waiting if we don't have any children
    if (!havekids || u->killed) {
      splx(e);
      return -1;
    }

    // wait for children to exit.  (See wakeup call in exit.)
    sleep(u);  // XXX DOC: wait-sleep
  }
}

// grow process by n bytes             XXX need to verify that u->sz is always at a 4 byte alignment  !!!!!
int sbrk(int n)
{
  uint osz, sz;
  if (!n) return u->sz;
  osz = sz = u->sz;
  if (n > 0) {
//    printf("growproc(%d)\n",n);
    if (!(sz = allocuvm(u->pdir, sz, sz + n, 0))) {
      printf("bad growproc!!\n"); //XXX
      return -1;
    }
  } else {
//    printf("shrinkproc(%d)\n",n);
//    if (sz + n < KRESERVED)
    if ((uint)(-n) > sz) { //XXX
      printf("bad shrinkproc!!\n"); //XXX
      return -1;
    }
    if (!(sz = deallocuvm(u->pdir, sz, sz + n))) return -1;
    pdir(V2P+(uint)(u->pdir));
  }
  u->sz = sz;
//  pdir(V2P+(u->pdir));
  return osz;
}

int ssleep(int n)
{
  uint ticks0; int e = splhi();

  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (u->killed) {
      splx(e);
      return -1;
    }
    sleep(&ticks);
  }
  splx(e);
  return 0;
}

// XXX HACK CODE (and all wrong) to bootstrap initial network functionality
enum { POLLIN = 1, POLLOUT = 2, POLLNVAL = 4 };
struct pollfd { int fd; short events, revents; };

int socket(int family, int type, int protocol)
{
  int fd, sd;
  struct file *f;

//  printf("socket(family=%d, type=%d, protocol=%d)\n", family, type, protocol);
  if (family != 2 || type != 1 || protocol != 0) return -1;

  if (!(f = filealloc()) || (fd = fdalloc(f)) < 0) {
    if (f) fileclose(f);
    return -1;
  }

  if ((sd = sockopen(family, type, protocol)) < 0) { u->ofile[fd] = 0; fileclose(f); return sd; }

  f->type = FD_SOCKET;
  f->readable = f->writable = 1;
  f->off = sd;
  return fd;
}

int poll(struct pollfd *pfd, uint n, int msec)
{
  int r, ev; struct file *f; struct pollfd *p, *pn;
  if (n && !mvalid(pfd, n * sizeof(struct pollfd))) return -1;
  pn = &pfd[n];
  for (;;)
  {
    r = 0;
    for (p = pfd; p != pn; p++)
    {
      if (p->fd < 0) continue;
      ev = 0;
      if (!(f = getf(p->fd))) ev = POLLNVAL;
      else if (p->events & POLLIN) {
        switch (f->type) {
        case FD_PIPE: if (f->pipe->nwrite != f->pipe->nread || !f->pipe->writeopen) ev = POLLIN; break;
        case FD_SOCKET: if (sockpoll(f->off)) ev = POLLIN; break;
        case FD_INODE:
          if ((f->ip->mode & S_IFMT) == S_IFCHR && f->ip->dir[0] == CONSOLE) {
            ilock(f->ip);
            if (input.r != input.w) ev = POLLIN;
            iunlock(f->ip);
          }
        }
      }
      if (p->revents = ev) { msec = 0; r++; }
    }
    if (!msec) break;
    ssleep(1);
    if (msec > 100) msec -= 100; // XXX this wrongly assumes one tick equals 100ms
    else if (msec > 0) msec = 0;
  }
  return r;
}
// XXX int connect(struct file *s, struct sockaddr *name, uint namelen)
int connect(int fd, uint *addr, int addrlen)
{
  struct file *f;
  if (!(f = getf(fd)) || addrlen < 8 || !mvalid(addr, addrlen)) return -1;
  return sockconnect(f->off, addr[0], addr[1]);
}

int sockbind(int fd, uint family_port, uint addr) { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(NET7); }
int bind(int fd, uint *addr, int addrlen)
{
  struct file *f;
  if (!(f = getf(fd)) || addrlen < 8 || !mvalid(addr, addrlen)) return -1;
  return sockbind(f->off, addr[0], addr[1]);
}
int socklisten() { asm(LL,8); asm(LBL,16); asm(NET8); }
int listen(int fd, int len)
{
  struct file *f;
  if (!(f = getf(fd))) return -1;
  return socklisten(f->off, len);
}
int sockaccept() { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(NET9); }
int accept(int fd, uint *addr, int *addrlen) // XXXX params!!!! accept(int,*|0,*|0)
{
  int sd;
  struct file *f;
  if (!(f = getf(fd))) return -1;
  
  while (!sockpoll(f->off)) ssleep(1); // XXX

  if ((sd = sockaccept(f->off, 0, 0)) < 0) return sd; // XXX null params for now
  
  if (!(f = filealloc()) || (fd = fdalloc(f)) < 0) { // XXX do this before sockaccept?
    if (f) fileclose(f);
    return -1;
  }

  f->type = FD_SOCKET;
  f->readable = f->writable = 1;
  f->off = sd;
  return fd;
}

// *** end of syscalls ***

// sleep on channel
sleep(void *chan)
{
  u->chan = chan;
  u->state = SLEEPING;
  sched();
  // tidy up
  u->chan = 0;
}

// wake up all processes sleeping on chan
wakeup(void *chan)
{
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan) p->state = RUNNABLE;
}

// a forked child's very first scheduling will swtch here
forkret()
{
  asm(POPA); asm(SUSP);
  asm(POPG);
  asm(POPF);
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

// Look in the process table for an UNUSED proc.  If found, change state to EMBRYO and initialize
// state required to run in the kernel.  Otherwise return 0.
struct proc *allocproc()
{
  struct proc *p; char *sp; int e = splhi();

  for (p = proc; p < &proc[NPROC]; p++)
    if (p->state == UNUSED) goto found;
  splx(e);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  splx(e);

  // allocate kernel stack leaving room for trap frame
  sp = (p->kstack = kalloc()) + PAGE - sizeof(struct trapframe);
  p->tf = (struct trapframe *)sp;
  
  // set up new context to start executing at forkret
  sp -= 8;
  *(uint *)sp = (uint)forkret;

  p->context = sp;
  return p;
}

// hand-craft the first process
init_start()
{
  char cmd[10], *argv[2];
  
  // no data/bss segment
  cmd[0] = '/'; cmd[1] = 'e'; cmd[2] = 't'; cmd[3] = 'c'; cmd[4] = '/';
  cmd[5] = 'i'; cmd[6] = 'n'; cmd[7] = 'i'; cmd[8] = 't'; cmd[9] = 0;
  
  argv[0] = cmd;
  argv[1] = 0;

  if (!init_fork()) init_exec(cmd, argv);
  init_exit(0); // become the idle task
}
init_fork() { asm(TRAP,S_fork); }
init_exec() { asm(LL,8); asm(LBL,16); asm(TRAP,S_exec); }
init_exit() { asm(LL,8); asm(TRAP,S_exit); }

userinit()
{
  char *mem;
  init = allocproc();
  init->pdir = memcpy(kalloc(), kpdir, PAGE);
  mem = memcpy(memset(kalloc(), 0, PAGE), (char *)init_start, (uint)userinit - (uint)init_start);
  mappage(init->pdir, 0, V2P+mem, PTE_P | PTE_W | PTE_U);

  init->sz = PAGE;
  init->tf->sp = PAGE;
  init->tf->fc = USER;
  init->tf->pc = 0;
  safestrcpy(init->name, "initcode", sizeof(init->name));
  init->cwd = namei("/");
  init->state = RUNNABLE;
}

// set up kernel page table
setupkvm()
{
  uint i, *pde, *pt;

  kpdir = memset(kalloc(), 0, PAGE); // kalloc returns physical addresses here (kfree wont work until later on)

  for (i=0; i<mem_sz; i += PAGE) {
    pde = &kpdir[(P2V+i) >> 22];
    if (*pde & PTE_P)
      pt = *pde & -PAGE;
    else
      *pde = (uint)(pt = memset(kalloc(), 0, PAGE)) | PTE_P | PTE_W;
    pt[((P2V+i) >> 12) & 0x3ff] = i | PTE_P | PTE_W;
  }
}

// return the address of the PTE in page table pd that corresponds to virtual address va
uint *walkpdir(uint *pd, uint va)
{
  uint *pde = &pd[va >> 22], *pt;

  if (!(*pde & PTE_P)) return 0;
  pt = P2V+(*pde & -PAGE);
  return &pt[(va >> 12) & 0x3ff];
}

// create PTE for a page
mappage(uint *pd, uint va, uint pa, int perm)
{
  uint *pde, *pte, *pt;

  if (*(pde = &pd[va >> 22]) & PTE_P)
    pt = P2V+(*pde & -PAGE);
  else
    *pde = (V2P+(uint)(pt = memset(kalloc(), 0, PAGE))) | PTE_P | PTE_W | PTE_U;
  pte = &pt[(va >> 12) & 0x3ff];
  if (*pte & PTE_P) { printf("*pte=0x%x pd=0x%x va=0x%x pa=0x%x perm=0x%x", *pte, pd, va, pa, perm); panic("remap"); }
  *pte = pa | perm;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(uint *pd, uint oldsz, uint newsz, int create) // XXX rename grow() ?
{
  uint va;
  if (newsz > USERTOP) return 0; // XXX make sure this never happens...
  if (newsz <= oldsz) panic("allocuvm: newsz <= oldsz"); // XXX do pre-checking in caller, no more post-checking needed
  
  va = (oldsz + PAGE-1) & -PAGE;
  while (va < newsz) {
    if (create)
      mappage(pd, va, V2P+(memset(kalloc(), 0, PAGE)), PTE_P | PTE_W | PTE_U);
    else
      mappage(pd, va, 0, PTE_W | PTE_U);
    va += PAGE;
  }  
  return newsz; // XXX not needed if never fails
}

// deallocate user pages to bring the process size from oldsz to newsz.
// oldsz and newsz need not be page-aligned, nor does newsz need to be less than oldsz.   XXXX wha why?
// oldsz can be larger than the actual process size.  Returns the new process size.
int deallocuvm(uint *pd, uint oldsz, uint newsz) // XXX rename shrink() ?? //XXX memset 0 top of partial page if present !!!
{
  uint va, *pde, *pte, *pt;

  if (newsz >= oldsz) return oldsz; // XXX maybe make sure this never happens

  va = newsz;
  if (va & (PAGE-1)) {
    memset(va, 0, PAGE - (va & (PAGE-1)));
    va = (va + PAGE-1) & -PAGE;
  }

  while(va < oldsz) {
    pde = &pd[(va >> 22) & 0x3ff]; //&pd[PDX(va)];
    if (*pde & PTE_P) { // XXX this may no longer be true if we are paging out pde/pte's?
      pt = P2V+(*pde & -PAGE);
      pte = &pt[(va >> 12) & 0x3ff]; // &pt[PTX(va)];

      if (*pte & PTE_P) {
        kfree(P2V+(*pte & -PAGE));
        *pte = 0;      
      }
      va += PAGE;
    }
    else
      va = (va + PAGE * 1024) & -(PAGE * 1024);
  }
  return newsz; // XXX not needed if never fails
}

// free a page table and all the physical memory pages in the user part
freevm(uint *pd)
{
  uint i;

  if (!pd) panic("freevm: no pd");
  deallocuvm(pd, USERTOP, 0);  // deallocate all user memory XXX do this more simply
  for (i = 0; i < ((USERTOP >> 22) & 0x3ff); i++) { // for (i = 0; i < PDX(USERTOP); i++)
    if (pd[i] & PTE_P) kfree(P2V+(pd[i] & -PAGE)); // deallocate all page table entries
  }
  kfree(pd); // deallocate page directory
}

// copy parent process page table for a child
uint *copyuvm(uint *pd, uint sz)
{
  uint va, *d, *pte;

  d = memcpy(kalloc(), kpdir, PAGE);
  for (va = 0; va < sz; va += PAGE) {
    if (!(pte = walkpdir(pd, va))) panic("copyuvm: pte should exist");

    if (*pte & PTE_P)
      mappage(d, va, V2P+(memcpy(kalloc(), P2V+(*pte & -PAGE), PAGE)), PTE_P | PTE_W | PTE_U); // XXX implement copy on write
    else
      mappage(d, va, 0, PTE_W | PTE_U);
  }
  return d;
}

swtch(int *old, int new) // switch stacks
{
  asm(LEA,0); // a = sp
  asm(LBL,8); // b = old
  asm(SX,0);  // *b = a
  asm(LL,16); // a = new
  asm(SSP);   // sp = a
}

scheduler()
{
  int n;
  
  for (n = 0; n < NPROC; n++) {  // XXX do me differently
    proc[n].next = &proc[(n+1)&(NPROC-1)];
    proc[n].prev = &proc[(n-1)&(NPROC-1)];
  }
  
  u = &proc[0];
  pdir(V2P+(uint)(u->pdir));
  u->state = RUNNING;
  swtch(&n, u->context);
  panic("scheduler returned!\n");
}

sched() // XXX redesign this better
{
  int n; struct proc *p;
//  if (u->state == RUNNING) panic("sched running");
//  if (lien()) panic("sched interruptible");
  p = u;
//  while (u->state != RUNNABLE) u = u->next;
  for (n=0;n<NPROC;n++) {
    u = u->next;
    if (u == &proc[0]) continue;
    if (u->state == RUNNABLE) goto found;
  }
  u = &proc[0];
  //printf("-");
  
found:
  u->state = RUNNING;
  if (p != u) {
    pdir(V2P+(uint)(u->pdir));
    //printf("+");
    swtch(&p->context, u->context);
  }
  //else printf("spin(%d)\n",u->pid);    XXX else do a wait for interrupt? (which will actually pend because interrupts are turned off here)
}

trap(uint *sp, double g, double f, int c, int b, int a, int fc, uint *pc)  
{
  uint va;
  switch (fc) {
  case FSYS: panic("FSYS from kernel");
  case FSYS + USER:
    if (u->killed) exit(-1);
    u->tf = &sp;
    switch (pc[-1] >> 8) {
    case S_fork:    a = fork(); break;
    case S_exit:    if (a < -99) printf("exit(%d)\n",a); exit(a); // XXX debug feature
    case S_wait:    a = wait(); break; // XXX args?
    case S_pipe:    a = pipe(a); break;
    case S_write:   a = write(a, b, c); break;
    case S_read:    a = read(a, b, c); break;
    case S_close:   a = close(a); break;
    case S_kill:    a = kill(a); break;
    case S_exec:    a = exec(a, b); break;
    case S_open:    a = open(a, b); break;
    case S_mknod:   a = mknod(a, b, c); break;
    case S_unlink:  a = unlink(a); break;
    case S_fstat:   a = fstat(a, b); break;
    case S_link:    a = link(a, b); break;
    case S_mkdir:   a = mkdir(a); break;
    case S_chdir:   a = chdir(a); break;
    case S_dup2:    a = dup2(a,b); break;
    case S_getpid:  a = u->pid; break;
    case S_sbrk:    a = sbrk(a); break;
    case S_sleep:   a = ssleep(a); break;
    case S_uptime:  a = ticks; break;
    case S_lseek:   a = lseek(a, b, c); break;
//  case S_mount:   a = mount(a, b, c); break;
//  case S_umount:  a = umount(a); break;
    case S_socket:  a = socket(a, b, c); break;
    case S_bind:    a = bind(a, b, c); break;
    case S_listen:  a = listen(a, b); break;
    case S_poll:    a = poll(a, b, c); break;
    case S_accept:  a = accept(a, b, c); break;
    case S_connect: a = connect(a, b, c); break;
    default: printf("pid:%d name:%s unknown syscall %d\n", u->pid, u->name, a); a = -1; break;
    }
    if (u->killed) exit(-1);
    return;
    
  case FMEM:          panic("FMEM from kernel");
  case FMEM   + USER: printf("FMEM + USER\n"); exit(-1);  // XXX psignal(SIGBUS)
  case FPRIV:         panic("FPRIV from kernel");
  case FPRIV  + USER: printf("FPRIV + USER\n"); exit(-1); // XXX psignal(SIGINS)
  case FINST:         panic("FINST from kernel");
  case FINST  + USER: printf("FINST + USER\n"); exit(-1); // psignal(SIGINS)
  case FARITH:        panic("FARITH from kernel");
  case FARITH + USER: printf("FARITH + USER\n"); exit(-1); // XXX psignal(SIGFPT)
  case FIPAGE:        printf("FIPAGE from kernel [0x%x]", lvadr()); panic("!\n");
  case FIPAGE + USER: printf("FIPAGE + USER [0x%x]", lvadr()); exit(-1); // XXX psignal(SIGSEG) or page in
  case FWPAGE:
  case FWPAGE + USER:
  case FRPAGE:        // XXX
  case FRPAGE + USER: // XXX
    if ((va = lvadr()) >= u->sz) exit(-1);
    pc--; // printf("fault"); // restart instruction
    mappage(u->pdir, va & -PAGE, V2P+(memset(kalloc(), 0, PAGE)), PTE_P | PTE_W | PTE_U);
    return;

  case FTIMER: 
  case FTIMER + USER: 
    ticks++;
    wakeup(&ticks);

    // force process exit if it has been killed and is in user space
    if (u->killed && (fc & USER)) exit(-1);
 
    // force process to give up CPU on clock tick
    if (u->state != RUNNING) { printf("pid=%d state=%d\n", u->pid, u->state); panic("!\n"); }        
    u->state = RUNNABLE;
    sched();

    if (u->killed && (fc & USER)) exit(-1);
    return;
    
  case FKEYBD:
  case FKEYBD + USER:
    consoleintr();
    return; //??XXX postkill?
  }
}

alltraps()
{
  asm(PSHA);
  asm(PSHB);
  asm(PSHC);
  asm(PSHF);
  asm(PSHG);
  asm(LUSP); asm(PSHA);
  trap();                // registers passed back out by magic reference :^O
  asm(POPA); asm(SUSP);
  asm(POPG);
  asm(POPF);
  asm(POPC);
  asm(POPB);
  asm(POPA);
  asm(RTI);
}

mainc()
{
  kpdir[0] = 0;          // don't need low map anymore
  consoleinit();         // console device
  ivec(alltraps);        // trap vector
  binit();               // buffer cache
  ideinit();             // disk
  stmr(128*1024);        // set timer
  userinit();            // first user process
  printf("Welcome!\n");
  scheduler();           // start running processes
}

main()
{
  int *ksp;              // temp kernel stack pointer
  static char kstack[256]; // temp kernel stack
  static int endbss;     // last variable in bss segment
    
  // initialize memory allocation
  mem_top = kreserved = ((uint)&endbss + PAGE + 3) & -PAGE; 
  mem_sz = msiz();
  
  // initialize kernel page table
  setupkvm();
  kpdir[0] = kpdir[(uint)USERTOP >> 22]; // need a 1:1 map of low physical memory for awhile

  // initialize kernel stack pointer
  ksp = ((uint)kstack + sizeof(kstack) - 8) & -8;
  asm(LL, 4);
  asm(SSP);

  // turn on paging
  pdir(kpdir);
  spage(1);
  kpdir = P2V+(uint)kpdir;
  mem_top = P2V+mem_top;

  // jump (via return) to high memory
  ksp = P2V+(((uint)kstack + sizeof(kstack) - 8) & -8);
  *ksp = P2V+(uint)mainc;
  asm(LL, 4);
  asm(SSP);
  asm(LEV);
}
