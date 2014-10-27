// em -- cpu emulator
//
// Usage:  em [-v] [-m memsize] [-f filesys] file
//
// Description:
//
// Written by Robert Swierczek

#include <u.h>
#include <libc.h>
#include <libm.h>
#include <net.h>

enum {
  MEM_SZ = 128*1024*1024, // default memory size of virtual machine (128M)
  TB_SZ  =     1024*1024, // page translation buffer array size (4G / pagesize)
  FS_SZ  =   4*1024*1024, // ram file system size (4M)
  TPAGES = 4096,          // maximum cached page translations
};

enum {           // page table entry flags
  PTE_P = 0x001, // Present
  PTE_W = 0x002, // Writeable
  PTE_U = 0x004, // User
  PTE_A = 0x020, // Accessed
  PTE_D = 0x040, // Dirty
};

enum {           // processor fault codes (some can be masked together)
  FMEM,          // bad physical address 
  FTIMER,        // timer interrupt
  FKEYBD,        // keyboard interrupt
  FPRIV,         // privileged instruction
  FINST,         // illegal instruction
  FSYS,          // software trap
  FARITH,        // arithmetic trap
  FIPAGE,        // page fault on opcode fetch
  FWPAGE,        // page fault on write
  FRPAGE,        // page fault on read
  USER = 16      // user mode exception 
};

uint verbose,    // chatty option -v
  mem, memsz,    // physical memory
  user,          // user mode
  iena,          // interrupt enable
  ipend,         // interrupt pending
  trap,          // fault code
  ivec,          // interrupt vector
  vadr,          // bad virtual address
  paging,        // virtual memory enabled
  pdir,          // page directory
  tpage[TPAGES], // valid page translations
  tpages,        // number of cached page translations
  *trk, *twk,    // kernel read/write page transation tables
  *tru, *twu,    // user read/write page transation tables
  *tr,  *tw;     // current read/write page transation tables

char *cmd;       // command name

void *new(int size)
{
  void *p;
  if ((p = sbrk((size + 7) & -8)) == (void *)-1) { dprintf(2,"%s : fatal: unable to sbrk(%d)\n", cmd, size); exit(-1); }
  return (void *)(((int)p + 7) & -8);
}

flush()
{
  uint v; 
//  static int xx; if (tpages >= xx) { xx = tpages; dprintf(2,"****** flush(%d)\n",tpages); }
//  if (verbose) printf("F(%d)",tpages);
  while (tpages) {
    v = tpage[--tpages];
    trk[v] = twk[v] = tru[v] = twu[v] = 0;    
  }
}

uint setpage(uint v, uint p, uint writable, uint userable)
{
  if (p >= memsz) { trap = FMEM; vadr = v; return 0; }
  p = ((v ^ (mem + p)) & -4096) + 1;
  if (!trk[v >>= 12]) {
    if (tpages >= TPAGES) flush();
    tpage[tpages++] = v;
  }
//  if (verbose) printf(".");
  trk[v] = p;
  twk[v] = writable ? p : 0;
  tru[v] = userable ? p : 0;
  twu[v] = (userable && writable) ? p : 0;
  return p;
}
  
uint rlook(uint v)
{
  uint pde, *ppde, pte, *ppte, q, userable;
//  dprintf(2,"rlook(%08x)\n",v);
  if (!paging) return setpage(v, v, 1, 1);
  pde = *(ppde = (uint *)(pdir + (v>>22<<2))); // page directory entry
  if (pde & PTE_P) {
    if (!(pde & PTE_A)) *ppde = pde | PTE_A;
    if (pde >= memsz) { trap = FMEM; vadr = v; return 0; }
    pte = *(ppte = (uint *)(mem + (pde & -4096) + ((v >> 10) & 0xffc))); // page table entry
    if ((pte & PTE_P) && ((userable = (q = pte & pde) & PTE_U) || !user)) {
      if (!(pte & PTE_A)) *ppte = pte | PTE_A;
      return setpage(v, pte, (pte & PTE_D) && (q & PTE_W), userable); // set writable after first write so dirty gets set
    }
  }
  trap = FRPAGE;
  vadr = v;
  return 0;
}

uint wlook(uint v)
{
  uint pde, *ppde, pte, *ppte, q, userable;
//  dprintf(2,"wlook(%08x)\n",v);
  if (!paging) return setpage(v, v, 1, 1);
  pde = *(ppde = (uint *)(pdir + (v>>22<<2))); // page directory entry
  if (pde & PTE_P) {
    if (!(pde & PTE_A)) *ppde = pde | PTE_A;
    if (pde >= memsz) { trap = FMEM; vadr = v; return 0; }
    pte = *(ppte = (uint *)(mem + (pde & -4096) + ((v >> 10) & 0xffc)));  // page table entry
    if ((pte & PTE_P) && (((userable = (q = pte & pde) & PTE_U) || !user) && (q & PTE_W))) {
      if ((pte & (PTE_D | PTE_A)) != (PTE_D | PTE_A)) *ppte = pte | (PTE_D | PTE_A);
      return setpage(v, pte, q & PTE_W, userable);
    }
  }
  trap = FWPAGE;
  vadr = v;
  return 0;
}

void cpu(uint pc, uint sp)
{
  uint a, b, c, ssp, usp, t, p, v, u, cycle, timer, timeout, xpc, ppc, delta;
  double f, g;
  int ir, kbchar;
  char ch;
  struct pollfd pfd;
  struct sockaddr_in addr;
  static char rbuf[4096]; // XXX
  
  a = b = c = cycle = timer = timeout = 0;
  delta = 4096;
  kbchar = -1;
  xpc = -1;

  for (;;) {
//    if (sp & 7) { dprintf(2,"stack pointer not a multiple of 8!\n", sp); goto fatal; }
    if (!(++cycle % delta)) { // XXX maybe dont check if last char pending?
      if (iena || !(ipend & FKEYBD)) { // XXX dont do this, use a small queue instead
        pfd.fd = 0;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 0) == 1 && read(0, &ch, 1) == 1) {
          kbchar = ch;
          if (kbchar == '`') { dprintf(2,"ungraceful exit. cycle = %u\n", cycle); return; }
          if (iena) { trap = FKEYBD; iena = 0; goto interrupt; }
          ipend |= FKEYBD;
        }
      }
      if (timeout) {
        timer += delta;
        if (timer >= timeout) { // XXX  // any interrupt actually!
//          dprintf(2,"timeout! timer=%d, timeout=%d\n",timer,timeout);
          timer = 0;      
          if (iena) { trap = FTIMER; iena = 0; goto interrupt; }
          ipend |= FTIMER;
        }
      }
    }
    if ((pc & (-4096 + 3)) != xpc) { // XXX just a simple optimization for now
      if (pc & 3) { dprintf(2,"pc not word aligned!\n", pc); goto fatal; } // XXX just reallign it?
      if (!(ppc = tr[(xpc = pc & -4096) >> 12]) && !(ppc = rlook(pc))) { xpc = -1; trap = FIPAGE; goto exception; }  // XXX be sure to invalidate xpc
      ppc--;
    }
    ir = *(int *)(pc ^ ppc);
    pc += 4;
    switch ((uchar)ir) {    
    case HALT: if (user || verbose) dprintf(2,"halt(%d) cycle = %u\n", a, cycle); return; // XXX should be supervisor!
    case IDLE: if (user) { trap = FPRIV; break; }
      if (!iena) { trap = FINST; break; } // XXX this will be fatal !!!
      for (;;) {
        pfd.fd = 0;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 0) == 1 && read(0, &ch, 1) == 1) {
          kbchar = ch;
          if (kbchar == '`') { dprintf(2,"ungraceful exit. cycle = %u\n", cycle); return; }
          trap = FKEYBD; 
          iena = 0;
          goto interrupt;
        }
        cycle += delta;
        if (timeout) {
          timer += delta;
          if (timer >= timeout) { // XXX  // any interrupt actually!
//        dprintf(2,"IDLE timeout! timer=%d, timeout=%d\n",timer,timeout);
            timer = 0;      
            trap = FTIMER;
            iena = 0;
            goto interrupt;
          }
        }
      }

    // memory -- designed to be restartable/continuable after exception/interrupt
    case MCPY: // while (c) { *a = *b; a++; b++; c--; }
      while (c) {
        if (!(t = tr[b >> 12]) && !(t = rlook(b))) goto exception;
        if (!(p = tw[a >> 12]) && !(p = wlook(a))) goto exception;
        if ((v = 4096 - (a & 4095)) > c) v = c;
        if ((u = 4096 - (b & 4095)) > v) u = v;
        memcpy((char *)(a ^ (p & -2)), (char *)(b ^ (t & -2)), u);
        a += u; b += u; c -= u;
//        if (!(++cycle % DELTA)) { pc -= 4; break; } XXX
      }
      continue;

    case MCMP: // for (;;) { if (!c) { a = 0; break; } if (*b != *a) { a = *b - *a; b += c; c = 0; break; } a++; b++; c--; }
      for (;;) {
        if (!c) { a = 0; break; }
        if (!(t = tr[b >> 12]) && !(t = rlook(b))) goto exception;
        if (!(p = tr[a >> 12]) && !(p = rlook(a))) goto exception;
        if ((v = 4096 - (a & 4095)) > c) v = c;
        if ((u = 4096 - (b & 4095)) > v) u = v;
        if (t = memcmp((char *)(a ^ (p & -2)), (char *)(b ^ (t & -2)), u)) { a = t; b += c; c = 0; break; }
        a += u; b += u; c -= u;
//        if (!(++cycle % DELTA)) { pc -= 4; break; } XXX
      }
      continue;

    case MCHR: // for (;;) { if (!c) { a = 0; break; } if (*a == b) { c = 0; break; } a++; c--; }
      for (;;) {
        if (!c) { a = 0; break; }
        if (!(p = tr[a >> 12]) && !(p = rlook(a))) goto exception;
        if ((u = 4096 - (a & 4095)) > c) u = c;
        if (t = (uint)memchr((char *)(v = a ^ (p & -2)), b, u)) { a += t - v; c = 0; break; } 
        a += u; c -= u;
//        if (!(++cycle % DELTA)) { pc -= 4; break; } XXX
      }
      continue;

    case MSET: // while (c) { *a = b; a++; c--; }
      while (c) {
        if (!(p = tw[a >> 12]) && !(p = wlook(a))) goto exception;
        if ((u = 4096 - (a & 4095)) > c) u = c;
        memset((char *)(a ^ (p & -2)), b, u);
        a += u; c -= u;
//        if (!(++cycle % DELTA)) { pc -= 4; break; } XXX
      }
      continue;

    // math
    case POW:  f = pow(f,g); continue;
    case ATN2: f = atan2(f,g); continue;
    case FABS: f = fabs(f); continue;
    case ATAN: f = atan(f); continue;
    case LOG:  if (f) f = log(f); continue; // XXX others?
    case LOGT: if (f) f = log10(f); continue; // XXX
    case EXP:  f = exp(f); continue;
    case FLOR: f = floor(f); continue;
    case CEIL: f = ceil(f); continue;
    case HYPO: f = hypot(f,g); continue;
    case SIN:  f = sin(f); continue;
    case COS:  f = cos(f); continue;
    case TAN:  f = tan(f); continue;
    case ASIN: f = asin(f); continue;
    case ACOS: f = acos(f); continue;
    case SINH: f = sinh(f); continue;
    case COSH: f = cosh(f); continue;
    case TANH: f = tanh(f); continue;
    case SQRT: f = sqrt(f); continue;
    case FMOD: f = fmod(f,g); continue;

    case ENT:  sp += ir>>8; continue;
    case LEV:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; pc = *(uint *)((v ^ p) & -8); sp += (ir>>8) + 8; continue;

    // jump
    case JMP:  pc += ir>>8; continue;
    case JMPI: if (!(p = tr[(v = pc + (ir>>8) + (a<<2)) >> 12]) && !(p = rlook(v))) break; pc += *(uint *)((v ^ p) & -4); continue;
    case JSR:  if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(uint *)((v ^ p) & -8) = pc; sp -= 8; pc += ir>>8; continue;
    case JSRA: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(uint *)((v ^ p) & -8) = pc; sp -= 8; pc = a; continue;

    // stack
    case PSHA: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -8) = a;     sp -= 8; continue;
    case PSHB: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -8) = b;     sp -= 8; continue;
    case PSHC: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -8) = c;     sp -= 8; continue;
    case PSHF: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(double *) ((v ^ p) & -8) = f;     sp -= 8; continue;
    case PSHG: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(double *) ((v ^ p) & -8) = g;     sp -= 8; continue;
    case PSHI: if (!(p = tw[(v = sp - 8) >> 12]) && !(p = wlook(v))) break; *(int *)    ((v ^ p) & -8) = ir>>8; sp -= 8; continue;    

    case POPA: if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) break; a = *(uint *)   ((sp ^ p) & -8); sp += 8; continue;
    case POPB: if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) break; b = *(uint *)   ((sp ^ p) & -8); sp += 8; continue;
    case POPC: if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) break; c = *(uint *)   ((sp ^ p) & -8); sp += 8; continue;
    case POPF: if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) break; f = *(double *) ((sp ^ p) & -8); sp += 8; continue;
    case POPG: if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) break; g = *(double *) ((sp ^ p) & -8); sp += 8; continue;

    // load effective address
    case LEA:  a = sp + (ir>>8); continue; 
    case LEAG: a = pc + (ir>>8); continue;

    // load a local
    case LL:   if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uint *)   ((v ^ p) & -4); continue;
    case LLS:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(short *)  ((v ^ p) & -2); continue;
    case LLH:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(ushort *) ((v ^ p) & -2); continue;
    case LLC:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(char *)   (v ^ p & -2); continue;
    case LLB:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uchar *)  (v ^ p & -2); continue;
    case LLD:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(double *) ((v ^ p) & -8); continue;
    case LLF:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(float *)  ((v ^ p) & -4); continue;

    // load a global
    case LG:   if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uint *)   ((v ^ p) & -4); continue;
    case LGS:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(short *)  ((v ^ p) & -2); continue;
    case LGH:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(ushort *) ((v ^ p) & -2); continue;
    case LGC:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(char *)   (v ^ p & -2); continue;
    case LGB:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uchar *)  (v ^ p & -2); continue;
    case LGD:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(double *) ((v ^ p) & -8); continue;
    case LGF:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(float *)  ((v ^ p) & -4); continue;

    // load a indexed
    case LX:   if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uint *)   ((v ^ p) & -4); continue;
    case LXS:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(short *)  ((v ^ p) & -2); continue;
    case LXH:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(ushort *) ((v ^ p) & -2); continue;
    case LXC:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(char *)   (v ^ p & -2); continue;
    case LXB:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = *(uchar *)  (v ^ p & -2); continue;
    case LXD:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(double *) ((v ^ p) & -8); continue;
    case LXF:  if (!(p = tr[(v = a + (ir>>8)) >> 12]) && !(p = rlook(v))) break; f = *(float *)  ((v ^ p) & -4); continue;

    // load a immediate
    case LI:   a = ir>>8; continue;
    case LHI:  a = a<<24 | (uint)ir>>8; continue;
    case LIF:  f = (ir>>8)/256.0; continue;

    // load b local
    case LBL:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uint *)   ((v ^ p) & -4); continue;
    case LBLS: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(short *)  ((v ^ p) & -2); continue;
    case LBLH: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(ushort *) ((v ^ p) & -2); continue;
    case LBLC: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(char *)   (v ^ p & -2); continue;
    case LBLB: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uchar *)  (v ^ p & -2); continue;
    case LBLD: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(double *) ((v ^ p) & -8); continue;
    case LBLF: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(float *)  ((v ^ p) & -4); continue;

    // load b global
    case LBG:  if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uint *)   ((v ^ p) & -4); continue;
    case LBGS: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(short *)  ((v ^ p) & -2); continue;
    case LBGH: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(ushort *) ((v ^ p) & -2); continue;
    case LBGC: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(char *)   (v ^ p & -2); continue;
    case LBGB: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uchar *)  (v ^ p & -2); continue;
    case LBGD: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(double *) ((v ^ p) & -8); continue;
    case LBGF: if (!(p = tr[(v = pc + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(float *)  ((v ^ p) & -4); continue;

    // load b indexed
    case LBX:  if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uint *)   ((v ^ p) & -4); continue;
    case LBXS: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(short *)  ((v ^ p) & -2); continue;
    case LBXH: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(ushort *) ((v ^ p) & -2); continue;
    case LBXC: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(char *)   (v ^ p & -2); continue;
    case LBXB: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; b = *(uchar *)  (v ^ p & -2); continue;
    case LBXD: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(double *) ((v ^ p) & -8); continue;
    case LBXF: if (!(p = tr[(v = b + (ir>>8)) >> 12]) && !(p = rlook(v))) break; g = *(float *)  ((v ^ p) & -4); continue;

    // load b immediate
    case LBI:  b = ir>>8; continue;
    case LBHI: b = b<<24 | (uint)ir>>8; continue;
    case LBIF: g = (ir>>8)/256.0; continue;

    // misc transfer
    case LCL:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; c = *(uint *) ((v ^ p) & -4); continue;

    case LBA:  b = a; continue;  // XXX need LAB, LAC to improve k.c  // or maybe a = a * imm + b ?  or b = b * imm + a ?
    case LCA:  c = a; continue;
    case LBAD: g = f; continue;

    // store a local
    case SL:   if (!(p = tw[(v = sp + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -4) = a; continue;
    case SLH:  if (!(p = tw[(v = sp + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(ushort *) ((v ^ p) & -2) = a; continue;
    case SLB:  if (!(p = tw[(v = sp + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uchar *)  (v ^ p & -2)   = a; continue;
    case SLD:  if (!(p = tw[(v = sp + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(double *) ((v ^ p) & -8) = f; continue;
    case SLF:  if (!(p = tw[(v = sp + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(float *)  ((v ^ p) & -4) = f; continue;

    // store a global
    case SG:   if (!(p = tw[(v = pc + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -4) = a; continue;
    case SGH:  if (!(p = tw[(v = pc + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(ushort *) ((v ^ p) & -2) = a; continue;
    case SGB:  if (!(p = tw[(v = pc + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uchar *)  (v ^ p & -2)   = a; continue;
    case SGD:  if (!(p = tw[(v = pc + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(double *) ((v ^ p) & -8) = f; continue;
    case SGF:  if (!(p = tw[(v = pc + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(float *)  ((v ^ p) & -4) = f; continue;

    // store a indexed
    case SX:   if (!(p = tw[(v = b + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uint *)   ((v ^ p) & -4) = a; continue;
    case SXH:  if (!(p = tw[(v = b + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(ushort *) ((v ^ p) & -2) = a; continue;
    case SXB:  if (!(p = tw[(v = b + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(uchar *)  (v ^ p & -2)   = a; continue;
    case SXD:  if (!(p = tw[(v = b + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(double *) ((v ^ p) & -8) = f; continue;
    case SXF:  if (!(p = tw[(v = b + (ir>>8)) >> 12]) && !(p = wlook(v))) break; *(float *)  ((v ^ p) & -4) = f; continue;
      
    // arithmetic
    case ADDF: f += g; continue;
    case SUBF: f -= g; continue;
    case MULF: f *= g; continue;
    case DIVF: if (g == 0.0) { trap = FARITH; break; } f /= g; continue; // XXX

    case ADD:  a += b; continue;
    case ADDI: a += ir>>8; continue;
    case ADDL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a += *(uint *)((v ^ p) & -4); continue;

    case SUB:  a -= b; continue;
    case SUBI: a -= ir>>8; continue;
    case SUBL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a -= *(uint *)((v ^ p) & -4); continue;

    case MUL:  a = (int)a * (int)b; continue; // XXX MLU ???
    case MULI: a = (int)a * (ir>>8); continue;
    case MULL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = (int)a * *(int *)((v ^ p) & -4); continue;

    case DIV:  if (!b) { trap = FARITH; break; } a = (int)a / (int)b; continue;
    case DIVI: if (!(t = ir>>8)) { trap = FARITH; break; } a = (int)a / (int)t; continue;
    case DIVL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break;
               if (!(t = *(uint *)((v ^ p) & -4))) { trap = FARITH; break; } a = (int)a / (int)t; continue;

    case DVU:  if (!b) { trap = FARITH; break; } a /= b; continue;
    case DVUI: if (!(t = ir>>8)) { trap = FARITH; break; } a /= t; continue;
    case DVUL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break;
               if (!(t = *(uint *)((v ^ p) & -4))) { trap = FARITH; break; } a /= t; continue;

    case MOD:  a = (int)a % (int)b; continue;
    case MODI: a = (int)a % (ir>>8); continue;
    case MODL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = (int)a % *(int *)((v ^ p) & -4); continue;

    case MDU:  a %= b; continue;
    case MDUI: a %= (ir>>8); continue;
    case MDUL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a %= *(uint *)((v ^ p) & -4); continue;

    case AND:  a &= b; continue;
    case ANDI: a &= ir>>8; continue;
    case ANDL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a &= *(uint *)((v ^ p) & -4); continue;

    case OR:   a |= b; continue;
    case ORI:  a |= ir>>8; continue;
    case ORL:  if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a |= *(uint *)((v ^ p) & -4); continue;

    case XOR:  a ^= b; continue;
    case XORI: a ^= ir>>8; continue;
    case XORL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a ^= *(uint *)((v ^ p) & -4); continue;

    case SHL:  a <<= b; continue;
    case SHLI: a <<= ir>>8; continue;
    case SHLL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a <<= *(uint *)((v ^ p) & -4); continue;

    case SHR:  a = (int)a >> (int)b; continue;
    case SHRI: a = (int)a >> (ir>>8); continue;
    case SHRL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a = (int)a >> *(int *)((v ^ p) & -4); continue;

    case SRU:  a >>= b; continue;
    case SRUI: a >>= ir>>8; continue;
    case SRUL: if (!(p = tr[(v = sp + (ir>>8)) >> 12]) && !(p = rlook(v))) break; a >>= *(uint *)((v ^ p) & -4); continue;

    // logical
    case EQ:   a = a == b; continue;
    case EQF:  a = f == g; continue;
    case NE:   a = a != b; continue;
    case NEF:  a = f != g; continue;
    case LT:   a = (int)a < (int)b; continue;
    case LTU:  a = a < b; continue;
    case LTF:  a = f < g; continue;
    case GE:   a = (int)a >= (int)b; continue;
    case GEU:  a = a >= b; continue;
    case GEF:  a = f >= g; continue;

    // branch
    case BZ:   if (!a)               pc += ir>>8; continue;
    case BZF:  if (!f)               pc += ir>>8; continue;
    case BNZ:  if (a)                pc += ir>>8; continue;
    case BNZF: if (f)                pc += ir>>8; continue;
    case BE:   if (a == b)           pc += ir>>8; continue;
    case BEF:  if (f == g)           pc += ir>>8; continue;
    case BNE:  if (a != b)           pc += ir>>8; continue;
    case BNEF: if (f != g)           pc += ir>>8; continue;
    case BLT:  if ((int)a < (int)b)  pc += ir>>8; continue;
    case BLTU: if (a < b)            pc += ir>>8; continue;
    case BLTF: if (f <  g)           pc += ir>>8; continue;
    case BGE:  if ((int)a >= (int)b) pc += ir>>8; continue;
    case BGEU: if (a >= b)           pc += ir>>8; continue;
    case BGEF: if (f >= g)           pc += ir>>8; continue;
    
    // conversion
    case CID:  f = (int)a; continue;
    case CUD:  f = a; continue;
    case CDI:  a = (int)f; continue;
    case CDU:  a = f; continue;

    // misc
    case BIN:  if (user) { trap = FPRIV; break; } a = kbchar; kbchar = -1; continue;  // XXX
    case BOUT: if (user) { trap = FPRIV; break; } if (a != 1) { dprintf(2,"bad write a=%d\n",a); return; } ch = b; a = write(a, &ch, 1); continue;
    case SSP:  sp = a; continue;

    case NOP:  continue;
    case CYC:  a = cycle; continue; // XXX protected?  XXX also need wall clock time instruction
    case MSIZ: if (user) { trap = FPRIV; break; } a = memsz; continue;
    
    case CLI:  if (user) { trap = FPRIV; break; } a = iena; iena = 0; continue;
    case STI:  if (user) { trap = FPRIV; break; } if (ipend) { trap = ipend & -ipend; ipend ^= trap; iena = 0; goto interrupt; } iena = 1; continue;

    case RTI:
      if (user) { trap = FPRIV; break; }
      if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) { dprintf(2,"RTI kstack fault\n"); goto fatal; }
      t  = *(uint *)((sp ^ p) & -8); sp += 8;
      if (!(p = tr[sp >> 12]) && !(p = rlook(sp))) { dprintf(2,"RTI kstack fault\n"); goto fatal; }
      pc = *(uint *)((sp ^ p) & -8); sp += 8;
      if (t & USER) { ssp = sp; sp = usp; user = 1; tr = tru; tw = twu; }
      if (!iena) { if (ipend) { trap = ipend & -ipend; ipend ^= trap; goto interrupt; } iena = 1; }
      continue;

    case IVEC: if (user) { trap = FPRIV; break; } ivec = a; continue;
    case PDIR: if (user) { trap = FPRIV; break; } if (a > memsz) { trap = FMEM; break; } pdir = (mem + a) & -4096; flush(); continue; // set page directory
    case SPAG: if (user) { trap = FPRIV; break; } if (a && !pdir) { trap = FMEM; break; } paging = a; flush(); continue; // enable paging
    
    case TIME: if (user) { trap = FPRIV; break; } 
       if (ir>>8) { dprintf(2,"timer%d=%u timeout=%u\n", ir>>8, timer, timeout); continue; }    // XXX undocumented feature!
       timeout = a; continue; // XXX cancel pending interrupts if disabled?

    // XXX need some sort of user mode thread locking functions to support user mode semaphores, etc.  atomic test/set?
    
    case LVAD: if (user) { trap = FPRIV; break; } a = vadr; continue;

    case TRAP: trap = FSYS; break;
    
    case LUSP: if (user) { trap = FPRIV; break; } a = usp; continue;
    case SUSP: if (user) { trap = FPRIV; break; } usp = a; continue;
    
    // networking -- XXX HACK CODE (and all wrong), but it gets some basic networking going...
    case NET1: if (user) { trap = FPRIV; break; } a = socket(a, b, c); continue; // XXX
    case NET2: if (user) { trap = FPRIV; break; } a = close(a); continue; // XXX does this block?
    case NET3: if (user) { trap = FPRIV; break; }
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = b & 0xFFFF;
      addr.sin_port = b >> 16;
      addr.sin_addr.s_addr = c;
      a = connect(a, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)); // XXX needs to be non-blocking
      continue;
    case NET4: if (user) { trap = FPRIV; break; }
      t = 0;
      // XXX if ((unknown || !ready) && !poll()) return -1;
      if ((int)c > 0) { if ((int)c > sizeof(rbuf)) c = sizeof(rbuf); a = c = read(a, rbuf, c); } else a = 0; // XXX uint or int??
      while ((int)c > 0) { 
        if (!(p = tw[b >> 12]) && !(p = wlook(b))) { dprintf(2,"unstable!!"); exit(9); } //goto exception; } XXX
        if ((u = 4096 - (b & 4095)) > c) u = c;
        memcpy((char *)(b ^ p & -2), &rbuf[t], u);
        t += u; b += u; c -= u;
      }
      continue;
    case NET5: if (user) { trap = FPRIV; break; }
      t = c;
      // XXX if ((unknown || !ready) && !poll()) return -1;
      while ((int)c > 0) {
        if (!(p = tr[b >> 12]) && !(p = rlook(b))) goto exception;
        if ((u = 4096 - (b & 4095)) > c) u = c;
        if ((int)(u = write(a, (char *)(b ^ p & -2), u)) > 0) { b += u; c -= u; } else { t = u; break; }
      }
      a = t;
      continue;
    case NET6: if (user) { trap = FPRIV; break; }
      pfd.fd = a;
      pfd.events = POLLIN;
      a = poll(&pfd, 1, 0); // XXX do something completely different
      continue;
    case NET7: if (user) { trap = FPRIV; break; }
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = b & 0xFFFF;
      addr.sin_port = b >> 16;
      addr.sin_addr.s_addr = c;
      a = bind(a, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
      continue;
    case NET8: if (user) { trap = FPRIV; break; }
      a = listen(a, b);
      continue;
    case NET9: if (user) { trap = FPRIV; break; }
      // XXX if ((unknown || !ready) && !poll()) return -1;
      a = accept(a, (void *)b, (void *)c); // XXX cant do this with virtual addresses!!!
      continue;
    
    default: trap = FINST; break;
    }
exception:
    if (!iena) { dprintf(2,"exception in interrupt handler\n"); goto fatal; }
interrupt:
    if (user) { usp = sp; sp = ssp; user = 0; tr = trk; tw = twk; trap |= USER; }
    sp -= 8; if (!(p = tw[sp >> 12]) && !(p = wlook(sp))) { dprintf(2,"kstack fault!\n"); goto fatal; }
    *(uint *)((sp ^ p) & -8) = pc;
    sp -= 8; if (!(p = tw[sp >> 12]) && !(p = wlook(sp))) { dprintf(2,"kstack fault\n"); goto fatal; }
    *(uint *)((sp ^ p) & -8) = trap;
    pc = ivec;
  }
fatal:
  dprintf(2,"processor halted! cycle = %u pc = %08x ir = %08x sp = %08x a = %d b = %d c = %d trap = %u\n", cycle, pc, ir, sp, a, b, c, trap);
}

usage()
{ 
  dprintf(2,"%s : usage: %s [-v] [-m memsize] [-f filesys] file\n", cmd, cmd);
  exit(-1);
}

int main(int argc, char *argv[])
{
  int i, f;
  struct { uint magic, bss, entry, flags; } hdr;
  char *file, *fs;
  struct stat st;
  
  cmd = *argv++;
  if (argc < 2) usage();
  file = *argv;
  memsz = MEM_SZ;
  fs = 0;
  while (--argc && *file == '-') {
    switch(file[1]) {
    case 'v': verbose = 1; break;
    case 'm': memsz = atoi(*++argv) * (1024 * 1024); argc--; break;
    case 'f': fs = *++argv; argc--; break;
    default: usage();
    }
    file = *++argv;
  }
  
  if (verbose) dprintf(2,"mem size = %u\n",memsz);
  mem = (((int) new(memsz + 4096)) + 4095) & -4096;
  
  if (fs) {
    if (verbose) dprintf(2,"%s : loading ram file system %s\n", cmd, fs);
    if ((f = open(fs, O_RDONLY)) < 0) { dprintf(2,"%s : couldn't open file system %s\n", cmd, fs); return -1; }
    if (fstat(f, &st)) { dprintf(2,"%s : couldn't stat file system %s\n", cmd, fs); return -1; }
    if ((i = read(f, (void*)(mem + memsz - FS_SZ), st.st_size)) != st.st_size) { dprintf(2,"%s : failed to read filesystem size %d returned %d\n", cmd, st.st_size, i); return -1; }
    close(f);
  }
  
  if ((f = open(file, O_RDONLY)) < 0) { dprintf(2,"%s : couldn't open %s\n", cmd, file); return -1; }
  if (fstat(f, &st)) { dprintf(2,"%s : couldn't stat file %s\n", cmd, file); return -1; }

  read(f, &hdr, sizeof(hdr));
  if (hdr.magic != 0xC0DEF00D) { dprintf(2,"%s : bad hdr.magic\n", cmd); return -1; }
  
  if (read(f, (void*)mem, st.st_size - sizeof(hdr)) != st.st_size - sizeof(hdr)) { dprintf(2,"%s : failed to read file %sn", cmd, file); return -1; }
  close(f);

//  if (verbose) dprintf(2,"entry = %u text = %u data = %u bss = %u\n", hdr.entry, hdr.text, hdr.data, hdr.bss);

  // setup virtual memory
  trk = (uint *) new(TB_SZ * sizeof(uint)); // kernel read table
  twk = (uint *) new(TB_SZ * sizeof(uint)); // kernel write table
  tru = (uint *) new(TB_SZ * sizeof(uint)); // user read table
  twu = (uint *) new(TB_SZ * sizeof(uint)); // user write table
  tr = trk;
  tw = twk;

  if (verbose) dprintf(2,"%s : emulating %s\n", cmd, file);
  cpu(hdr.entry, memsz - FS_SZ);
  return 0;
}

