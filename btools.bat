del gld.exe fsd.exe term.exe
gcc -o gld -O3 -m32 mingw/gld.c -lwsock32 -lgdi32
gcc -o fsd -O3 -m32 mingw/fsd.c -lwsock32
gcc -o term -O3 -m32 -Imingw -Iroot/lib root/bin/term.c
