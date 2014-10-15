// ls -- list contents of directory
//
// Usage:  ls [path] ...
//
// Description:
//   ls lists the contents of the directory for each path argument.

#include <u.h>
#include <libc.h>
#include <dir.h>

int verbose;

void ls(char *path)
{
  char buf[PATH_MAX], *p;
  struct stat st;
  DIR *d;
  struct dirent *de;
  
  if (stat(path, &st)) { dprintf(2, "ls: cannot stat %s\n", path); return; }
  
  switch (st.st_mode & S_IFMT) {
  case S_IFREG:
    for (p = path + strlen(path); p >= path && *p != '/'; p--) ;
    if (verbose) printf("%4d %2d %6d ", st.st_ino, st.st_nlink, st.st_size);
    printf("%s\n",p+1);
    break;
  
  case S_IFDIR:
//    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){ // XXX
//      dprintf(2,"ls: path too long\n");
//      break;
//    }
    strcpy(buf, path); // XXX add length checks
    p = buf+strlen(buf); 
    *p++ = '/';
    if (!(d = opendir(path))) dprintf(2,"opendir failed\n");   
    while (de = readdir(d)) {
      strcpy(p, de->d_name);
      if (stat(buf, &st)) { dprintf(2,"ls: cannot stat %s\n", buf); continue; }
      if (verbose) printf("%4d %2d %6d ", st.st_ino, st.st_nlink, st.st_size);
      printf(((st.st_mode & S_IFMT) == S_IFDIR && strcmp(de->d_name,".") && strcmp(de->d_name,"..")) ? "%s/\n" : "%s\n", de->d_name);
    }
    closedir(d);
    break;
  }
}

int main(int argc, char *argv[])
{
  int i;
  if (argc > 1 && !strcmp(argv[1],"-l")) {
    verbose = 1;
    argc--; argv++;
  }
  if (argc == 1)
    ls(".");
  else {
    for (i=1; i<argc; i++) ls(argv[i]);
  }
  return 0;
}
