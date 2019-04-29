#include <u.h>
#include <libc.h>

YY(char *s)
{
  printf("YY(%s)\n", s);
}

#define YY printf
#define XX YY("hello world.\n")
#define const


const int main()
{
  XX;
#define XX YY("hello\n")
  XX;
#undef YY
  XX;
#define XX YY("done\n")
  XX;
#define YY printf
  XX;
  return 0;
}
