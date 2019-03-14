#!/bin/sh
rm -f xc xem xeu xhello emhello euhello c em eu
gcc -o xc -O3 -m32 -Ilinux -Iroot/lib root/bin/c.c
gcc -o xem -O3 -m32 -Ilinux -Iroot/lib root/bin/em.c -lm
gcc -o xeu -O3 -m32 -Ilinux -Iroot/lib root/bin/eu.c -lm
./xc -o c -Iroot/lib root/bin/c.c
./xc -o em -Iroot/lib root/bin/em.c
./xc -o eu -Iroot/lib root/bin/eu.c
./xc -o euhello -Iroot/lib root/usr/euhello.c
./xc -o emhello -Iroot/lib root/usr/emhello.c
./xeu euhello
./xem emhello
./xeu eu euhello
./xeu em emhello
./xeu -v eu -v eu -v eu -v euhello
./xeu -v eu -v eu -v em -v emhello