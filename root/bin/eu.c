// eu -- user mode cpu emulator
//
// Usage:  eu [-v] file ...
//
// Description:
//
// Written by Robert Swierczek

#include <u.h>
#include <libc.h>
#include <libm.h>
#include <net.h>

enum { STACKSZ = 8*1024*1024 }; // user stack size (8M)

int verbose;
char *cmd;

int cpu(uint pc, int argc, char **argv)
{
  uint a, b, c, sp, cycle = 0;
  int ir;
  double f, g;

  sp = ((uint)sbrk(STACKSZ) + STACKSZ - 28) & -8;
  
  ((uint *)sp)[0] = sp + 24;
  ((uint *)sp)[2] = argc;
  ((uint *)sp)[4] = (uint) argv;
  ((uint *)sp)[6] = TRAP | (S_exit<<8); // call exit if main returns

  for (;;) {   
    if (sp & 7) { dprintf(2,"stack pointer not a multiple of 8! sp = %u\n", sp); return -1; }
    cycle++;    
    ir = *(int *)pc;
    pc += 4;
    switch ((uchar)ir) {
    case HALT: dprintf(2,"halted! a = %d cycle = %u\n", a, cycle); return -1; // XXX supervisor mode

    // memory
    case MCPY: memcpy((void *)a, (void *)b, c); a += c; b += c; c = 0; continue;
    case MCMP: a = memcmp((void *)a, (void *)b, c); b += c; c = 0; continue;
    case MCHR: if (a = (uint)memchr((void *)a, b, c)) c = 0; continue;
    case MSET: memset((void *)a, b, c); a += c; c = 0; continue;

    // math
    case POW:  f = pow(f,g); continue;
    case ATN2: f = atan2(f,g); continue;
    case FABS: f = fabs(f); continue;
    case ATAN: f = atan(f); continue;
    case LOG:  f = log(f); continue;
    case LOGT: f = log10(f); continue;
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

    // procedure linkage
    case ENT:  sp += ir>>8; continue;
    case LEV:  sp += ir>>8; pc = *(uint *)sp; sp += 8; continue;

    // jump
    case JMP:  pc += ir>>8; continue;
    case JMPI: pc += ((uint *)(pc + (ir>>8)))[a]; continue;
    case JSR:  sp -= 8; *(uint *)sp = pc; pc += ir>>8; continue;
    case JSRA: sp -= 8; *(uint *)sp = pc; pc = a; continue;

    // stack
    case PSHA: sp -= 8; *(uint *)sp = a; continue;
    case PSHB: sp -= 8; *(uint *)sp = b; continue;
    case PSHC: sp -= 8; *(uint *)sp = c; continue;
    case PSHF: sp -= 8; *(double *)sp = f; continue;
    case PSHG: sp -= 8; *(double *)sp = g; continue;
    case PSHI: sp -= 8; *(uint *)sp = ir>>8; continue;

    case POPA: a = *(uint *)sp; sp += 8; continue;
    case POPB: b = *(uint *)sp; sp += 8; continue;
    case POPC: c = *(uint *)sp; sp += 8; continue;
    case POPF: f = *(double *)sp; sp += 8; continue;
    case POPG: g = *(double *)sp; sp += 8; continue;

    // load effective address
    case LEA:  a = sp + (ir>>8); continue; 
    case LEAG: a = pc + (ir>>8); continue;

    // load a local
    case LL:   a = *(uint *)   (sp + (ir>>8)); continue;
    case LLS:  a = *(short *)  (sp + (ir>>8)); continue;
    case LLH:  a = *(ushort *) (sp + (ir>>8)); continue;
    case LLC:  a = *(char *)   (sp + (ir>>8)); continue;
    case LLB:  a = *(uchar *)  (sp + (ir>>8)); continue;
    case LLD:  f = *(double *) (sp + (ir>>8)); continue;
    case LLF:  f = *(float *)  (sp + (ir>>8)); continue;

    // load a global
    case LG:   a = *(uint *)   (pc + (ir>>8)); continue;
    case LGS:  a = *(short *)  (pc + (ir>>8)); continue;
    case LGH:  a = *(ushort *) (pc + (ir>>8)); continue;
    case LGC:  a = *(char *)   (pc + (ir>>8)); continue;
    case LGB:  a = *(uchar *)  (pc + (ir>>8)); continue;
    case LGD:  f = *(double *) (pc + (ir>>8)); continue;
    case LGF:  f = *(float *)  (pc + (ir>>8)); continue;

    // load a indexed
    case LX:   a = *(uint *)   (a + (ir>>8)); continue;
    case LXS:  a = *(short *)  (a + (ir>>8)); continue;
    case LXH:  a = *(ushort *) (a + (ir>>8)); continue;
    case LXC:  a = *(char *)   (a + (ir>>8)); continue;
    case LXB:  a = *(uchar *)  (a + (ir>>8)); continue;
    case LXD:  f = *(double *) (a + (ir>>8)); continue;
    case LXF:  f = *(float *)  (a + (ir>>8)); continue;

    // load a immediate
    case LI:   a = ir>>8; continue;
    case LHI:  a = a<<24 | (uint)ir>>8; continue;
    case LIF:  f = (ir>>8)/256.0; continue;

    // load b local
    case LBL:  b = *(uint *)   (sp + (ir>>8)); continue;
    case LBLS: b = *(short *)  (sp + (ir>>8)); continue;
    case LBLH: b = *(ushort *) (sp + (ir>>8)); continue;
    case LBLC: b = *(char *)   (sp + (ir>>8)); continue;
    case LBLB: b = *(uchar *)  (sp + (ir>>8)); continue;
    case LBLD: g = *(double *) (sp + (ir>>8)); continue;
    case LBLF: g = *(float *)  (sp + (ir>>8)); continue;

    // load b global
    case LBG:  b = *(uint *)   (pc + (ir>>8)); continue;
    case LBGS: b = *(short *)  (pc + (ir>>8)); continue;
    case LBGH: b = *(ushort *) (pc + (ir>>8)); continue;
    case LBGC: b = *(char *)   (pc + (ir>>8)); continue;
    case LBGB: b = *(uchar *)  (pc + (ir>>8)); continue;
    case LBGD: g = *(double *) (pc + (ir>>8)); continue;
    case LBGF: g = *(float *)  (pc + (ir>>8)); continue;

    // load b indexed
    case LBX:  b = *(uint *)   (b + (ir>>8)); continue;
    case LBXS: b = *(short *)  (b + (ir>>8)); continue;
    case LBXH: b = *(ushort *) (b + (ir>>8)); continue;
    case LBXC: b = *(char *)   (b + (ir>>8)); continue;
    case LBXB: b = *(uchar *)  (b + (ir>>8)); continue;
    case LBXD: g = *(double *) (b + (ir>>8)); continue;
    case LBXF: g = *(float *)  (b + (ir>>8)); continue;

    // load b immediate
    case LBI:  b = ir>>8; continue;
    case LBHI: b = b<<24 | (uint)ir>>8; continue;
    case LBIF: g = (ir>>8)/256.0; continue;

    // misc transfer
    case LCL:  c = *(uint *)(sp + (ir>>8)); continue;
    case LBA:  b = a; continue;
    case LCA:  c = a; continue;
    case LBAD: g = f; continue;

    // store a local
    case SL:   *(uint *)   (sp + (ir>>8)) = a; continue;
    case SLH:  *(ushort *) (sp + (ir>>8)) = a; continue;
    case SLB:  *(uchar *)  (sp + (ir>>8)) = a; continue;
    case SLD:  *(double *) (sp + (ir>>8)) = f; continue;
    case SLF:  *(float *)  (sp + (ir>>8)) = f; continue;

    // store a global
    case SG:   *(uint *)   (pc + (ir>>8)) = a; continue;
    case SGH:  *(ushort *) (pc + (ir>>8)) = a; continue;
    case SGB:  *(uchar *)  (pc + (ir>>8)) = a; continue;
    case SGD:  *(double *) (pc + (ir>>8)) = f; continue;
    case SGF:  *(float *)  (pc + (ir>>8)) = f; continue;

    // store a indexed
    case SX:   *(uint *)   (b + (ir>>8)) = a; continue;
    case SXH:  *(ushort *) (b + (ir>>8)) = a; continue;
    case SXB:  *(uchar *)  (b + (ir>>8)) = a; continue;
    case SXD:  *(double *) (b + (ir>>8)) = f; continue;
    case SXF:  *(float *)  (b + (ir>>8)) = f; continue;
      
    // arithmetic
    case ADDF: f += g; continue;
    case SUBF: f -= g; continue;
    case MULF: f *= g; continue;
    case DIVF: f /= g; continue;

    case ADD:  a += b; continue;
    case ADDI: a += ir>>8; continue;
    case ADDL: a += *(uint *)(sp + (ir>>8)); continue;

    case SUB:  a -= b; continue;
    case SUBI: a -= ir>>8; continue;
    case SUBL: a -= *(uint *)(sp + (ir>>8)); continue;

    case MUL:  a *= b; continue;
    case MULI: a *= ir>>8; continue;
    case MULL: a *= *(uint *)(sp + (ir>>8)); continue;

    case DIV:  a = (int)a / (int)b; continue;
    case DIVI: a = (int)a / (ir>>8); continue;
    case DIVL: a = (int)a / *(int *)(sp + (ir>>8)); continue;

    case DVU:  a /= b; continue;
    case DVUI: a /= ir>>8; continue;
    case DVUL: a /= *(uint *)(sp + (ir>>8)); continue;

    case MOD:  a = (int)a % (int)b; continue;
    case MODI: a = (int)a % (ir>>8); continue;
    case MODL: a = (int)a % *(int *)(sp + (ir>>8)); continue;

    case MDU:  a %= b; continue;
    case MDUI: a %= ir>>8; continue;
    case MDUL: a %= *(uint *)(sp + (ir>>8)); continue;

    case AND:  a &= b; continue;
    case ANDI: a &= ir>>8; continue;
    case ANDL: a &= *(uint *)(sp + (ir>>8)); continue;

    case OR:   a |= b; continue;
    case ORI:  a |= ir>>8; continue;
    case ORL:  a |= *(uint *)(sp + (ir>>8)); continue;

    case XOR:  a ^= b; continue;
    case XORI: a ^= ir>>8; continue;
    case XORL: a ^= *(uint *)(sp + (ir>>8)); continue;

    case SHL:  a <<= b; continue;
    case SHLI: a <<= ir>>8; continue;
    case SHLL: a <<= *(uint *)(sp + (ir>>8)); continue;

    case SHR:  a = (int)a >> (int)b; continue;
    case SHRI: a = (int)a >> (ir>>8); continue;
    case SHRL: a = (int)a >> *(int *)(sp + (ir>>8)); continue;

    case SRU:  a >>= b; continue;
    case SRUI: a >>= ir>>8; continue;
    case SRUL: a >>= *(uint *)(sp + (ir>>8)); continue;

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
    case BLTF: if (f < g)            pc += ir>>8; continue;
    case BGE:  if ((int)a >= (int)b) pc += ir>>8; continue;
    case BGEU: if (a >= b)           pc += ir>>8; continue;
    case BGEF: if (f >= g)           pc += ir>>8; continue;
    
    // conversion
    case CID:  f = (int)a; continue;
    case CUD:  f = a; continue;
    case CDI:  a = (int)f; continue;
    case CDU:  a = f; continue;

    // misc
    case SSP:  sp = a; continue;
    case NOP:  continue;
    case CYC:  a = cycle; continue;

    case TRAP:
      switch (ir>>8) {
      case S_fork:    a = fork();                          continue; // fork()
      case S_exit:    if (verbose) dprintf(2,"exit(%d) cycle = %u\n", a, cycle); return a; // exit(rc)
      case S_wait:    a = wait();                          continue; // wait()
      case S_pipe:    a = pipe((void *)a);                 continue; // pipe(&fd)
      case S_write:   a = write(a, (void *)b, c);          continue; // write(fd, p, n)
      case S_read:    a = read(a, (void *)b, c);           continue; // read(fd, p, n)
      case S_close:   a = close(a);                        continue; // close(fd)
      case S_kill:    a = kill(a);                         continue; // kill(pid)
      case S_exec:    a = exec((void *)a, (void *)b);      continue; // exec(path, argv)
      case S_open:    a = open((void *)a, b);              continue; // open(path, mode)
      case S_mknod:   a = mknod((void *)a, b, c);          continue; // mknod(path, mode, dev)
      case S_unlink:  a = unlink((void *)a);               continue; // unlink(path)
      case S_fstat:   a = fstat(a, (void *)b);             continue; // fstat(fd, p)
      case S_link:    a = link((void *)a, (void *)b);      continue; // link(old, new)
      case S_mkdir:   a = mkdir((void *)a);                continue; // mkdir(path);
      case S_chdir:   a = chdir((void *)a);                continue; // chdir(path)
      case S_dup2:    a = dup2(a, b);                      continue; // dup2(fd, nfd)
      case S_getpid:  a = getpid();                        continue; // getpid()
      case S_sbrk:    a = (uint)sbrk(a);                   continue; // sbrk(size)
      case S_sleep:   a = sleep(a);                        continue; // sleep(msec) XXX msec vs sec
      case S_uptime:  a = uptime();                        continue; // uptime()
      case S_lseek:   a = lseek(a, b, c);                  continue; // lseek(fd, pos, whence)
      case S_mount:   a = mount((void *)a, (void *)b, c);  continue; // mount(spec, dir, rwflag)
      case S_umount:  a = umount((void *)a);               continue; // umount(spec)
      case S_socket:  a = socket(a, b, c);                 continue; // socket(family, type, protocol)
      case S_bind:    a = bind(a, (void *)b, c);           continue; // bind(fd, addr, addrlen)
      case S_listen:  a = listen(a, b);                    continue; // listen(fd, len)
      case S_poll:    a = poll((void *)a, b, c);           continue; // poll(pfd, n, msec)
      case S_accept:  a = accept(a, (void *)b, (void *)c); continue; // accept(fd, addr, addrlen)
      case S_connect: a = connect(a, (void *)b, c);        continue; // connect(fd, addr, addrlen)

//      case S_shutdown:
//      case S_getsockopt:
//      case S_setsockopt:
//      case S_getpeername:
//      case S_getsockname:
      
      default: dprintf(2,"unsupported trap cycle = %u pc = %08x ir = %08x a = %d b = %d c = %d", cycle, pc, ir, a, b, c); return -1;
      }
    default:   dprintf(2,"unknown instruction cycle = %u pc = %08x ir = %08x\n", cycle, pc, ir); return -1;
    }
  }
}

usage()
{
  dprintf(2,"%s : usage: %s [-v] file ...\n", cmd, cmd);
  exit(-1);
}

int main(int argc, char *argv[])
{
  int f, gs, rc;
  struct { uint magic, bss, entry, flags; } hdr;
  char *file;
  struct stat st;

  cmd = *argv;
  if (argc < 2) usage();
  file = *++argv;
  verbose = 0;
  while (--argc && *file == '-') {
    switch(file[1]) {
    case 'v': verbose = 1; break;
    default: usage();
    }
    file = *++argv;
  }

  if ((f = open(file, O_RDONLY)) < 0) { dprintf(2,"%s : couldn't open %s\n", cmd, file); return -1; }
  if (fstat(f, &st)) { dprintf(2,"%s : couldn't stat file %s\n", cmd, file); return -1; }

  read(f, &hdr, sizeof(hdr));
  if (hdr.magic != 0xC0DEF00D) { dprintf(2,"%s : bad hdr.magic\n", cmd); return -1; }
  
  gs = (int)sbrk(st.st_size - sizeof(hdr) + hdr.bss);
  read(f, (void *)gs, st.st_size - sizeof(hdr));
  close(f);

  if (verbose) dprintf(2,"%s : emulating %s\n", cmd, file);
  rc = cpu(gs + hdr.entry, argc, argv);
  if (verbose) dprintf(2,"%s : %s returned %d.\n", cmd, file, rc);
  return rc;
}
