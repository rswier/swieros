#!/bin/sh
rm -f gld fsd term
gcc -o gld -O3 -m32 linux/gld.c -lX11 -lGL
#gcc -o fsd -O3 -m32 linux/fsd.c
gcc -o term -O3 -m32 -Ilinux -Iroot/lib root/bin/term.c
