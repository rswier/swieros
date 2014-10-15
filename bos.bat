del c.exe em.exe os0 os1 os2 os3
gcc -o c -O3 -m32 -Imingw -Iroot/lib root/bin/c.c
gcc -o em -O3 -m32 -Imingw -Iroot/lib root/bin/em.c
c -o os0 -Iroot/lib root/usr/os/os0.c
c -o os1 -Iroot/lib root/usr/os/os1.c
c -o os2 -Iroot/lib root/usr/os/os2.c
c -o os3 -Iroot/lib root/usr/os/os3.c
em os0
em os1
em os2
em os3
