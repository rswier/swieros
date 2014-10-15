del c.exe em.exe eu.exe hello emhello euhello c em eu
gcc -o c -O3 -m32 -Imingw -Iroot/lib root/bin/c.c
gcc -o em -O3 -m32 -Imingw -Iroot/lib root/bin/em.c
gcc -o eu -O3 -m32 -Imingw -Iroot/lib root/bin/eu.c
c -o c -Iroot/lib root/bin/c.c
c -o em -Iroot/lib root/bin/em.c
c -o eu -Iroot/lib root/bin/eu.c
c -o euhello -Iroot/lib root/usr/euhello.c
c -o emhello -Iroot/lib root/usr/emhello.c
eu euhello
em emhello
eu eu euhello
eu em emhello
eu -v eu -v eu -v eu -v euhello
eu -v eu -v eu -v em -v emhello