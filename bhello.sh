#!/bin/sh
rm -f xc xem xeu xhello hello emhello euhello hello.txt emhello.txt euhello.txt
gcc -o xc -O3 -Ilinux -Iroot/lib root/bin/c.c
gcc -o xem -O3 -m32 -Ilinux -Iroot/lib root/bin/em.c -lm
gcc -o xeu -O3 -m32 -Ilinux -Iroot/lib root/bin/eu.c -lm
gcc -o xhello -O3 -m32 -Ilinux -Iroot/lib root/usr/hello.c
./xc -s -Iroot/lib root/usr/hello.c > hello.txt
./xc -s -Iroot/lib root/usr/euhello.c > euhello.txt
./xc -s -Iroot/lib root/usr/emhello.c > emhello.txt
./xc -o hello -Iroot/lib root/usr/hello.c
./xc -o euhello -Iroot/lib root/usr/euhello.c
./xc -o emhello -Iroot/lib root/usr/emhello.c
./xhello
./xeu hello
./xeu euhello
./xem emhello
