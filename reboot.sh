#!/bin/sh
rm -f root/etc/os
./xc -o root/etc/os -Iroot/lib root/etc/os.c
./xem -f fs.img root/etc/os
