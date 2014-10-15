// httpd -- tiny http daemon
//
// Usage:  httpd [port]
//
// Description:
//   httpd launches a web server allowing remote access to the current directory.
//   Based on the Nigel's Web server nweb22.c written by Nigel Griffiths.

#include <u.h>
#include <libc.h>
#include <net.h>

enum { BUFSIZE = 8096 };

char notfound[] =
  "HTTP/1.1 404 Not Found\n"
  "Content-Length: 136\n"
  "Connection: close\n"
  "Content-Type: text/html\n\n"
  "<html><head>\n"
  "<title>404 Not Found</title>\n"
  "</head><body>\n"
  "<h1>Not Found</h1>\n"
  "The requested URL was not found on this server.\n"
  "</body></html>\n";
  
char forbidden[] =
  "HTTP/1.1 403 Forbidden\n"
  "Content-Length: 185\n"
  "Connection: close\n"
  "Content-Type: text/html\n\n"
  "<html><head>\n"
  "<title>403 Forbidden</title>\n"
  "</head><body>\n"
  "<h1>Forbidden</h1>\n"
  "The requested URL, file type or operation is not allowed on this simple static file webserver.\n"
  "</body></html>\n";

void fatal(char *s)
{
  printf("fatal: %s\n", s);
  exit(-1);
}

int web(int sd)
{
  int fd, i, j, r, len;
  char *ty;
  static char buffer[BUFSIZE+1];

  buffer[0] = 0; // XXXX hack to suppress unstable!! spew from em.c
  if ((r = read(sd, buffer, BUFSIZE)) <= 0) { write(sd, forbidden, strlen(forbidden)); return -1; } // read request

  for (i=0; i<r; i++) if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = '*'; // remove CF and LF characters
  if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4) ) { write(sd, forbidden, strlen(forbidden)); return -1; }
  for (i=4; i<BUFSIZE; i++) if (buffer[i] == ' ') { buffer[i] = 0; break; } // null terminate after the second space
  for (j=0; j<i-1; j++) if (buffer[j] == '.' && buffer[j+1] == '.') { write(sd, forbidden, strlen(forbidden)); return -1; } // check for illegal parent directory use ..
  if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6) ) strcpy(buffer, "GET /index.html"); // convert no filename to index file

  // work out the file type and check we support it
  len = strlen(buffer);
  if      (!strcmp(&buffer[len - 4], ".gif"))  ty = "image/gif";
  else if (!strcmp(&buffer[len - 4], ".jpg"))  ty = "image/jpg";
  else if (!strcmp(&buffer[len - 5], ".jpeg")) ty = "image/jpeg";
  else if (!strcmp(&buffer[len - 4], ".png"))  ty = "image/png";
  else if (!strcmp(&buffer[len - 4], ".ico"))  ty = "image/ico";
  else if (!strcmp(&buffer[len - 4], ".zip"))  ty = "image/zip";
  else if (!strcmp(&buffer[len - 3], ".gz"))   ty = "image/gz";
  else if (!strcmp(&buffer[len - 4], ".tar"))  ty = "image/tar";
  else if (!strcmp(&buffer[len - 2], ".c"))    ty = "text/c";
  else if (!strcmp(&buffer[len - 4], ".htm"))  ty = "text/html";
  else if (!strcmp(&buffer[len - 5], ".html")) ty = "text/html";
  else { write(sd, forbidden, strlen(forbidden)); return -1; }
    
  if ((fd = open(&buffer[5], O_RDONLY)) == -1) { write(sd, notfound, strlen(notfound)); return -1; }
  len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  dprintf(sd, "HTTP/1.1 200 OK\nServer: httpd/1.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n", len, ty);

  while ((r = read(fd, buffer, BUFSIZE)) > 0) write(sd, buffer, r);
  //sleep(1); // XXX allow socket to drain before signalling the socket is closed
  close(sd);
  return 0;
}

int main(int argc, char *argv[])
{
  int ld, sd, r;
  static struct sockaddr_in addr;

  if ((ld = socket(AF_INET, SOCK_STREAM,0)) < 0) fatal("socket()");
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((argc == 2) ? atoi(argv[1]) : 80);
  if (bind(ld, (struct sockaddr *) &addr, sizeof(addr))) fatal("bind()");
  if (listen(ld, 1) < 0) fatal("listen()");

  for (;;) {
    if ((sd = accept(ld, 0, 0)) < 0) fatal("accept()");
    if ((r = fork()) < 0) fatal("fork()");
    if (!r) {
      close(ld);
      return web(sd);
    }
    close(sd);
  }
}
