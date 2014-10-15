// ftpd -- nonsecure file transfer protocol server daemon
//
// Usage:  ftpd [-v] [-g] [-p port]
//
// Description:
//   ftpd is a minimal FTP server program.  Use with caution, as it provides
//   no authentication or security of any sort.  Useful for single machine
//   host/guest file transfers.
//
//   The following options are supported:
//
//   -v   Verbose output
//   -g   Get only (disallow upload)
//   -p   Specify port number (default 21)

// Based on the free ftpdmin v0.96 by Matthias Wandel.

#include <u.h>
#include <libc.h>
#include <net.h>
#include <dir.h>
#include <ctype.h>

enum { MAX_PATH = 512 }; // XXX

struct sockaddr_in xaddr; // transfer address
int xaddrlen;
char xbuf[256*1024]; // transfer buffer
int cd; // control socket
int getonly, verbose;
char cmd[6];

int command(char *arg)
{
  char buf[501];
  int len, i, j;

  for (len = 0;;) {
    if (len >= sizeof(buf) - 1) return 0;
    if ((i = read(cd, buf + len, sizeof(buf)-1 - len)) <= 0) return 0;
    buf[len += i] = 0;
    if (strchr(buf, '\r')) break;
  }

  for (i=0; i < sizeof(cmd)-1; i++) {
    if (!isalpha(buf[i])) break;
    cmd[i] = toupper(buf[i]);
  }
  cmd[i] = 0;

  j = 0;
  if (buf[i++] == ' ') {
    for (; j<500; j++) {
      if (buf[i+j] < 32) break;
      arg[j] = buf[i+j];
    }
  }
  arg[j] = 0;

  if (verbose) dprintf(2,"%s %s\n", cmd, arg);
  return 1;
}

void reply(char *r)
{
  char buf[MAX_PATH+20];
  if (verbose) dprintf(2,"  %s\n",r);
  sprintf(buf, "%s\r\n", r);
  write(cd, buf, strlen(buf));
}

void nlst(char *name, int listlong, int usectl)
{
  int xd, i;
  char buf[500], timestr[20];
  DIR *d;
  struct dirent *de;
  struct stat st;
//  struct tm *tm;

  if (usectl)
    xd = cd;
  else {
    reply("150 Opening connection");
    if ((xd = socket(AF_INET, SOCK_STREAM, 0)) < 0 || connect(xd, (struct sockaddr *)&xaddr, xaddrlen) < 0)
      { reply("550 connect() error"); return; }
  }

  if (*name == 0) name = ".";
  else if (*name == '-') { 
    for (i=1; name[i]; i++) { if (name[i] == 'l') listlong = 1; }
    name = ".";
  }

  if (!(d = opendir(name))) { reply("550 connect() error"); return; }
  while (de = readdir(d)) {
    if (de->d_name[0] == '.' && (!de->d_name[1] || (de->d_name[1] == '.' && !de->d_name[2]))) continue;
    if (listlong) { // ls -lA
      sprintf(buf, "%s/%s", name, de->d_name);
      if (stat(buf, &st)) continue;
      if ((st.st_mode & S_IFMT) != S_IFDIR && (st.st_mode & S_IFMT) != S_IFREG) continue;
//      tm = localtime(&st.st_mtime);
//      strftime(timestr, 20, "%b %d  %Y", tm);
//      sprintf(timestr, "%.3s %02d  %d", "JanFebMarAprMayJunJulAugSepOctNovDec" + 3*tm->tm_mon, tm->tm_mday, 1900 + tm->tm_year);
      strcpy(timestr, "Jan 01  2000");

//      sprintf(buf,"%c%s   1 root  root  %7u %s %s\r\n", (S_ISDIR(st.st_mode) ? 'd' : '-'), ((st.st_mode & S_IWUSR) ? "rw-rw-rw-" : "r--r--r--"), st.st_size, timestr, de->d_name);
      sprintf(buf,"%c%s   1 root  root  %7u %s %s\r\n", ((st.st_mode & S_IFMT) == S_IFDIR ? 'd' : '-'), "rw-rw-rw-", st.st_size, timestr, de->d_name);
            
    } else { // ls
      sprintf(buf, "%s\r\n",de->d_name);
    }
    if (verbose) write(2, buf, strlen(buf));
    write(xd, buf, strlen(buf));
  }
  closedir(d);

  if (!usectl) {
    close(xd);
    reply("226 Transfer Complete");
  }
}

void retr(char *name)
{
  int file, xd, size; // XXX fix size < 0 nonsense logic

  if ((file = open(name, O_RDONLY)) < 0) { reply("550 open() error"); return; }

  reply("150 Opening BINARY mode data connection");
  if ((xd = socket(AF_INET, SOCK_STREAM, 0)) < 0 || connect(xd, (struct sockaddr *)&xaddr, xaddrlen) < 0)
    { reply("550 connect() error"); return; }

  for (size=1;size > 0;) {
    if ((size = read(file, xbuf, sizeof(xbuf))) < 0) { reply("550 read() error"); break; }
    if (write(xd, xbuf, size) < 0) {
      if (verbose) dprintf(2,"send failed\n");
      reply("426 Broken pipe") ;
      size = -1;
    }
  }

  if (size >= 0) reply("226 Transfer Complete");   // XXX fix size < 0 nonsense logic
  close(xd);
  close(file);
}

void stor(char *name)
{
  int file, xd, size; // XXX fix size < 0 nonsense logic

  if ((file = open(name, O_WRONLY | O_CREAT | O_TRUNC)) < 0) { reply("550 open() error"); return; } // XXX third arg?

  reply("150 Opening BINARY mode data connection");
  if ((xd = socket(AF_INET, SOCK_STREAM, 0)) < 0 || connect(xd, (struct sockaddr *)&xaddr, xaddrlen) < 0)
    { reply("550 connect() error"); return; }

  for (size=1;size >= 0;) {
//    if ((size = read(xd, xbuf, sizeof(xbuf))) < 0) { if (verbose) dprintf(2,"read failed\n"); reply("550 read() error"); break; }
    if ((size = read(xd, xbuf, 256)) < 0) { if (verbose) dprintf(2,"read failed\n"); reply("550 read() error"); break; } // XXX buffer sizes are all crapped up right now
    if (size == 0) { reply("226 Transfer Complete"); break; }
    if (write(file, xbuf, size) != size) { reply("550 write() error"); break; }
  }

  close(xd);

  close(file);
}

void mdtm(char *name)
{
  struct stat s;
//  struct tm *t;
  char buf[50];

  if (stat(name, &s)) { reply("550 stat() error"); return; }
  if ((s.st_mode & S_IFMT) != S_IFREG) { reply("550 not a plain file."); return; } // it's a directory
//  t = localtime(&s.st_mtime); // or gmtime()
//  sprintf(buf, "213 %04d%02d%02d%02d%02d%02d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  strcpy(buf, "213 20010101010101");
  reply(buf);
}

void siz(char *name)
{
  struct stat s;
  char buf[50];

  if (stat(name, &s)) { reply("550 stat() error"); return; }
  if ((s.st_mode & S_IFMT) != S_IFREG) { reply("550 not a plain file."); return; } // it's a directory
  sprintf(buf, "213 %u",s.st_size);
  reply(buf);
}

void port(char *arg)
{
  char *a; int h1,h2,h3,h4,p1,p2;
//  sscanf(arg,"%d,%d,%d,%d,%d,%d",&h1,&h2,&h3,&h4,&p1,&p2); // XXX
  h1 = h2 = h3 = h4 = p1 = p2 = 0;
  while ('0' <= *arg && *arg <= '9') h1 = h1 * 10 + *arg++ - '0'; arg++;
  while ('0' <= *arg && *arg <= '9') h2 = h2 * 10 + *arg++ - '0'; arg++;
  while ('0' <= *arg && *arg <= '9') h3 = h3 * 10 + *arg++ - '0'; arg++;
  while ('0' <= *arg && *arg <= '9') h4 = h4 * 10 + *arg++ - '0'; arg++;
  while ('0' <= *arg && *arg <= '9') p1 = p1 * 10 + *arg++ - '0'; arg++;
  while ('0' <= *arg && *arg <= '9') p2 = p2 * 10 + *arg++ - '0'; arg++;
  
  a = (char *) &xaddr.sin_addr; a[0] = h1; a[1] = h2; a[2] = h3; a[3] = h4;
  a = (char *) &xaddr.sin_port; a[0] = p1; a[1] = p2;
  if (verbose) dprintf(2,"PORT(%d,%d)\n",xaddr.sin_addr.s_addr,xaddr.sin_port);
  reply("200 PORT command successful");
}

int mv(char *from, char *to)
{
  if (link(from, to)) { if (verbose) dprintf(2,"mv: cannot link to %s\n", to); return -1; }
  if (unlink(from)) { if (verbose) dprintf(2,"mv: cannot unlink %s\n", from); return -1; }
  return 0;
}

void handler()
{
  char arg[MAX_PATH+10], buf[MAX_PATH+10];

  chdir("/");

  // get default data address for transfer socket
  xaddrlen = sizeof(xaddr);
//  if (getsockname(cd, (struct sockaddr *)&xaddr, &xaddrlen) < 0) {
//    if (verbose) dprintf(2,"getsockname() failed\n");
//    reply("500 getsockname failed");
//    goto end;
//  }
//  if (verbose) dprintf(2,"getsockname(%d,%d)\n",xaddr.sin_addr.s_addr,xaddr.sin_port);
  xaddr.sin_family = AF_INET;

  reply("220 ftpd ready");
  while (command(arg)) {
    if (!strncmp(cmd, "USER", 4)) reply("331 pretend login accepted");
    else if (!strncmp(cmd, "PASS", 4)) reply("230 fake user logged in");
    else if (!strncmp(cmd, "SYST", 4)) reply("215 ftpd");
    else if (!strncmp(cmd, "PASV", 4)) reply("550 Permission denied");
    else if (!strncmp(cmd, "XPWD", 4) || !strncmp(cmd, "PWD")) { // print working directory
      getcwd(arg, sizeof(arg));
      sprintf(buf,"257 \"%s\"", arg);
      reply(buf);
    }
    else if (!strncmp(cmd, "NLST", 4)) nlst(arg, 0, 0); // request directory, names only
    else if (!strncmp(cmd, "LIST", 4)) nlst(arg, 1, 0); // request directory, long version
    else if (!strncmp(cmd, "STAT", 4)) nlst(arg, 1, 1); // like LIST, but use control connection
    else if (!strncmp(cmd, "DELE", 4)) {
      if (unlink(arg)) reply("550 unlink() error");
      else reply("250 DELE command successful.");
    }
    else if (!strncmp(cmd, "RMD", 4) || !strncmp(cmd, "XRMD", 4)) {  // XXX for now
//      if (getonly) reply("550 Permission denied");
//      else if (rmdir(arg)) reply("550 rmdir() error");
//      else reply("250 RMD command successful");
      reply("550 Permission denied");
    }
    else if (!strncmp(cmd, "MKD", 4) || !strncmp(cmd, "XMKD", 4)) {
      if (getonly) reply("550 Permission denied");
      else if (mkdir(arg)) reply("550 mkdir() error");
      else reply("257 Directory created");
    }
    else if (!strncmp(cmd, "RNFR", 4)) { // rename from
      strcpy(buf, arg);
      reply("350 File Exists");
    }
    else if (!strncmp(cmd, "RNTO", 4)) { // rename to, must be immediately preceeded by RNFR
      if (getonly) reply("550 Permission denied");
      else if (mv(buf, arg)) reply("550 rename() error");
      else reply("250 RNTO command successful");
    }
    else if (!strncmp(cmd, "ABOR", 4)) reply("226 Aborted");
    else if (!strncmp(cmd, "SIZE", 4)) siz(arg);
    else if (!strncmp(cmd, "MDTM", 4)) mdtm(arg);
    else if (!strncmp(cmd, "CWD", 4)) { // change working directory
      if (chdir(arg)) reply("550 Could not change directory");
      else reply("250 CWD command successful");
    }
    else if (!strncmp(cmd, "TYPE", 4)) reply("200 Type set to I"); // accept file TYPE commands, but ignore
    else if (!strncmp(cmd, "NOOP", 4)) reply("200 OK");
    else if (!strncmp(cmd, "PORT", 4)) port(arg); // set the TCP/IP address for transfers
    else if (!strncmp(cmd, "RETR", 4)) retr(arg); // retrieve File and send it
    else if (!strncmp(cmd, "STOR", 4)) { if (getonly) reply("553 Permission denied"); else stor(arg); } // store the file
    else if (!strncmp(cmd, "QUIT", 4)) { reply("221 goodbye"); break; }
    else reply("500 command not implemented");
  }
  if (verbose) dprintf(2,"Closing control connection\n");
  close(cd);
}

void usage(void)
{
  dprintf(2, "usage: ftpd [-v] [-g] [-p port]\n");
  exit(-1);
}

int main(int argc, char **argv)
{
  int ld, port = 21, i;
  static struct sockaddr_in addr;
   
  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i],"-p")) { if (argc < i+2) usage(); port = atoi(argv[++i]); }
    else if (!strcmp(argv[i],"-g")) getonly = 1;
    else if (!strcmp(argv[i],"-v")) verbose = 1;
    else usage();
  }

  chdir("/");

  if ((ld = socket(AF_INET, SOCK_STREAM, 0)) < 0) { dprintf(2,"socket() failed\n"); return -1; }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  
  if (bind(ld, (struct sockaddr *)&addr, sizeof(addr)) < 0) { dprintf(2,"bind() failed\n"); return -1; }
  if (listen(ld, 1) < 0) { dprintf(2,"listen() failed\n"); return -1; }
  if (verbose) dprintf(2,"ftpd ready to accept connections on port %d\n", port);

  for (;;) {
    if ((cd = accept(ld, 0, 0)) < 0) { dprintf(2,"accept() failed\n"); return -1; }
    if (verbose) dprintf(2,"accepted new ftp connection\n");
    if ((i = fork()) < 0) { dprintf(2,"fork() failed\n"); return -1; }
    if (!i) {
      close(0);
      close(ld);
      handler();
      return 0;
    }
    close(cd);
  }
}
