// sh -- command interpreter shell
//
// Usage:  sh ...
//
// Description:
//   sh is the standard command interpreter.

#include <u.h>
#include <libc.h>

// Parsed command representation
enum { EXEC = 1, REDIR, PIPE, LIST, BACK };
enum { MAXARGS = 32 };
int interactive;

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;
  char buf[100];

  if (cmd == 0)
    exit(0);
  
  switch (cmd->type) {
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if (ecmd->argv[0] == 0)
      exit(0);
    else if (strchr(ecmd->argv[0], '/'))
      exec(ecmd->argv[0], ecmd->argv);
    else {
      memcpy(buf, "/bin/", 5); 
      strcpy(buf+5, ecmd->argv[0]);
      exec(buf, ecmd->argv);
    }
    dprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if (open(rcmd->file, rcmd->mode) < 0) {
      dprintf(2, "open(%s,%d) failed\n", rcmd->file, rcmd->mode);
      exit(0);
    }
    if (rcmd->mode == O_WRONLY | O_CREAT) {
      if (lseek(rcmd->fd, 0, SEEK_END) == -1) {
        dprintf(2, "lseek(%s, 0, SEEK_END) failed\n", rcmd->file);
        exit(0);
      }
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if (fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if (pipe(p) < 0)
      panic("pipe");
    if (fork1() == 0) {
      dup2(p[1],1);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if (fork1() == 0) {
      dup2(p[0],0);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;
    
  case BACK:
    bcmd = (struct backcmd*)cmd;
    if (fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit(0);
}

int getchar()
{
  static uchar buf[BUFSIZ], *p;
  static int n;
  if (!n && (n = read(0, (p = buf), sizeof(buf))) <= 0) { n = 0; return -1; }
  n--;
  return *p++;
}

char *gets(char *s)
{
  char *p; int c;
  for (p = s;;) {
    if ((c = getchar()) == -1) return 0;
    if (c == '\b') {
      if (p > s) { if (interactive) write(1,"\b \b",3); p--; }
    } else if (c == '\r' || c == '\n' || c >= ' ') {
      if (interactive) write(1,&c,1);
      if (c == '\r' && interactive) write(1,"\n",1);
      if (c == '\n' || c == '\r') break;
      *p++ = c;
    }
  }
  *p = 0;
  return s;
}

int getcmd(char *buf)
{
  if (interactive) dprintf(1, "$ ");
  return gets(buf) ? 0 : -1;
}

char *whitespace;
char *symbols;

int main(int argc, char *argv[])
{
  static char buf[BUFSIZ];
  int fd;
  
  whitespace = " \t\r\n\v";
  symbols = "<|>&;()";

  if (argc == 2 && !strcmp(argv[1],"-i")) interactive = 1;

  // Assumes three file descriptors open.
  while ((fd = open("/dev/console", O_RDWR)) >= 0) {
    if (fd >= 3) {
      close(fd);
      break;
    }
  }
  
  // Read and run input commands.
  while (getcmd(buf) >= 0) {
    if (!memcmp(buf,"cd ",3)) {
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      if (chdir(buf+3) < 0)
        dprintf(2, "cannot cd %s\n", buf+3);
      continue;
    }
    else if (!strcmp(buf,"exit")) 
      return 0; // XXX should allow return code (exit [n])
    if (fork1() == 0)
      runcmd(parsecmd(buf));
    wait();
  }
  return 0;
}

void panic(char *s)
{
  dprintf(2, "%s\n", s);
  exit(0);
}

int fork1(void)
{
  int pid;
  
  if ((pid = fork()) == -1)
    panic("fork");
  return pid;
}

// Constructors

struct cmd *execcmd(void)
{
  struct execcmd *cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd *listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd *backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}

// Parsing
int gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s = *ps;
  int ret;
  
  while (s < es && strchr(whitespace, *s))
    s++;
  if (q)
    *q = s;
  ret = *s;
  switch (*s) {
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if (*s == '>') {
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if (eq)
    *eq = s;
  
  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int peek(char **ps, char *es, char *toks)
{
  char *s = *ps;

  while (s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd *parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if (s != es) {
    dprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd *parseline(char **ps, char *es)
{
  struct cmd *cmd = parsepipe(ps, es);
  while (peek(ps, es, "&")) {
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if (peek(ps, es, ";")) {
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd *parsepipe(char **ps, char *es)
{
  struct cmd *cmd = parseexec(ps, es);
  if (peek(ps, es, "|")) {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while (peek(ps, es, "<>")) {
    tok = gettoken(ps, es, 0, 0);
    if (gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch (tok) {
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT | O_TRUNC, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT, 1);
      break;
    }
  }
  return cmd;
}

struct cmd *parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if (!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if (!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd *parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  if (peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while (!peek(ps, es, "|)&;")) {
    if ((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if (tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if (argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    return 0;
  
  switch (cmd->type) {
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;
    
  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
