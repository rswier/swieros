del c.exe em.exe mkfs.exe root\bin\c root\etc\os root\etc\sfs.img fs.img
gcc -o c -O3 -m32 -Imingw -Iroot/lib root/bin/c.c
gcc -o em -O3 -m32 -Imingw -Iroot/lib root/bin/em.c
gcc -o mkfs -O3 -m32 -Imingw -Iroot/lib root/etc/mkfs.c
c -o root/bin/c -Iroot/lib root/bin/c.c
c -o root/etc/os -Iroot/lib root/etc/os.c
mkfs sfs.img root
copy sfs.img root\etc
del sfs.img
mkfs fs.img root
em -f fs.img root/etc/os
