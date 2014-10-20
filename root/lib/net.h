// net.h 

int socket()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_socket);  }
int bind()    { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_bind);    }
int listen()  { asm(LL,8); asm(LBL,16);              asm(TRAP,S_listen);  }
int accept()  { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_accept);  }
int connect() { asm(LL,8); asm(LBL,16); asm(LCL,24); asm(TRAP,S_connect); }

typedef uint   in_addr_t;
typedef ushort in_port_t;
typedef ushort sa_family_t;

struct in_addr { in_addr_t s_addr; };
struct sockaddr_in {
  sa_family_t    sin_family;
  in_port_t      sin_port;
  struct in_addr sin_addr;
  uchar          sin_zero[8];
};

enum {
  PF_INET = 2,
  AF_INET = 2,
  SOCK_STREAM = 1,
  INADDR_ANY = 0,
};

uint htonl(uint a)
{
  uint bigend = 1;
  return ((char *)&bigend)[3] ? a : ((a >> 24) | ((a >> 8) & 0xff00) | ((a << 8) & 0xff0000) | (a << 24));
}
ushort htons(ushort a)
{
  uint bigend = 1;
  return ((char *)&bigend)[3] ? a : ((a >> 8) | (a << 8));
}
