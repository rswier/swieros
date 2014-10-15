del c.exe em.exe eu.exe hello.exe hello emhello euhello hello.txt emhello.txt euhello.txt
gcc -o c -O3 -m32 -Imingw -Iroot/lib root/bin/c.c
gcc -o em -O3 -m32 -Imingw -Iroot/lib root/bin/em.c
gcc -o eu -O3 -m32 -Imingw -Iroot/lib root/bin/eu.c
gcc -o hello -O3 -m32 -Imingw -Iroot/lib root/usr/hello.c
c -s -Iroot/lib root/usr/hello.c > hello.txt
c -s -Iroot/lib root/usr/euhello.c > euhello.txt
c -s -Iroot/lib root/usr/emhello.c > emhello.txt
c -o hello -Iroot/lib root/usr/hello.c
c -o euhello -Iroot/lib root/usr/euhello.c
c -o emhello -Iroot/lib root/usr/emhello.c
hello
eu hello
eu euhello
em emhello
