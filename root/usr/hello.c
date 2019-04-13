#include <u.h>
#include <libc.h>

#define YY printf
#define XX YY("hello world.\n")

int main()
{
  XX;
  return 0;
}
