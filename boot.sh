#!/bin/sh
rm -f xc xem xmkfs root/bin/c root/etc/os root/etc/sfs.img fs.img
gcc -o xc -O3 -m32 -Ilinux -Iroot/lib root/bin/c.c
gcc -o xem -O3 -m32 -Ilinux -Iroot/lib root/bin/em.c -lm
gcc -o xmkfs -O3 -m32 -Ilinux -Iroot/lib root/etc/mkfs.c
./xc -o root/bin/c -Iroot/lib root/bin/c.c
./xc -o root/etc/os -Iroot/lib root/etc/os.c
./xmkfs sfs.img root
mv sfs.img root/etc/.
./xmkfs fs.img root
./xem -f fs.img root/etc/os
