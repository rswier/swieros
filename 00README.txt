00README.txt:

Welcome to my synthetic CPU emulator, C compiler, and self-contained operating system.
Here you will find the full source to everything you need to build and run a tiny Unix-like
operating system (based on xv6) in an emulated environment under Windows and Linux.

You will need to supply a (Intel-endian) PC running Linux or Windows and the MinGW C compiler.
To exercise the graphics capability of the OS, you will need a graphics card capable of running
OpenGL applications.

Using a graphical FTP client such as WinSCP is handy under Windows, but not required.
Another useful but not required download is ANSICON by Jason Hood, which provides ANSI escape
sequence handling for the Windows console.

You can go for broke by typing "boot", or it might be better to peek inside some of the script
files to see how everything works (for each xyz.bat, there is an equivalent xyz.sh for Linux.)

Here's what is in the package:

    00README.txt   - This file.
    bhello.bat     - Test the compiler and emulator with a few hello worlds (good starting point.)
    bos.bat        - Demonstrate some techniques that the OS uses.
    recurse.bat    - Demonstrate recursive emulation.
    btools.bat     - Builds a graphics server (gld), file server (fsd) and a terminal client (term).
    boot.bat       - Boot-straps the compiler, RAM file system, and boots into the OS.
    reboot.bat     - Quicker boot into the OS without rebuilding everything.
    cleanup.bat    - Clean up everything to a pre-built state.

    mingw/*.h      - MinGW/Win32 versions of headers in root/lib/ allowing certain programs
                     to be built for both Win32 and the OS from a single source.
    mingw/gld.c    - Graphics server daemon (see btools.bat) similar to an X Server, but for
                     OpenGL applications.  Run gld.exe and boot.bat in separate console windows.
                     This server is needed by most programs in the root/usr/demo/ directory.
    mingw/fsd.c    - Experimental remote file system server.
    linux/*        - Linux versions of header files and servers (counterpart to mingw/*).

    root/          - Root directory used to create the OS RAM file system image (fs.img).

    root/bin/      - Directory for system commands:
    root/bin/c.c       - C compiler (use the -s option to study the generated code.)
    root/bin/edit.c    - Text editor (under development, currently pretty useless.)
    root/bin/em.c      - CPU emulator.  Full user and supervisor modes with virtual memory.
    root/bin/eu.c      - User mode only emulator.  OS Traps get passed back to the host.
    root/bin/ftpd.c    - File transfer protocol daemon server.
    root/bin/halt.c    - Quick and dirty shutdown.
    root/bin/httpd.c   - Tiny web server.
    root/bin/man.c     - Manual pages for commands in root/bin/
    root/bin/sh.c      - Command shell (provides the $ command line prompt.)
    root/bin/shd.c     - Command shell daemon server (allows remote access using term.exe)
    root/bin/term.c    - Remote terminal client for contacting shell daemon (see above.)
    root/bin/vt.c      - Xterm-like terminal client (requires gld.exe to be running, see above.)

    root/dev/      - Directory for special device files.

    root/etc/      - Directory for special OS files:
    root/etc/init.c    - First process launched on boot.  Passes control to the command shell.
    root/etc/mkfs.c    - Make a file system from the specified directory.
    root/etc/os.c      - The OS kernel (based on xv6 with heavy modificatons.)

    root/lib/      - Directory for system library headers.  Time for some background...

A major goal of the project is to make things simple and well integrated.  The system
compiles and executes programs on-the-fly, so the file system contains mostly just source
code and a few executables such as the C compiler itself (you can always manually compile
a program if desired.)

For compilation simplicity and speed, there is no pre-processor (however #include is supported.)
There is also no linker for creating executables from separately compiled source objects.
This means that everything a program depends on must either be in it's one and only .c file or the
included header files.  Normally it is bad practice to put a function body in a .h file,
but the header files in root/lib are equivalent to libraries themselves, so it works, sort of.
Maybe C will get a reasonable module system someday.

There is an 8MB limit on the total size of a program's text, data and bss segments (a compiler
simplification), and other minor compiler incompatibilities that are easily avoided (for instance,
there are some restrictions on initializers since there is no loader address fix-up phase.)

Everything is slimmed down to promote rapid prototyping of the OS, compiler, and CPU architecture.
The not-invented-here syndrome is avoided as much as possible by implemented subsets of well known
API's (with minor deviations where needed for simplicity.)  The project is a starting point
for endless extensions, improvements, and new radical ideas.

...back to the file tree:

    root/lib/curses.h  - Simple subset of UNIX Curses.
    root/lib/forms.h   - Mostly compatible subset of the XForms GUI/Widget library.
    root/lib/gl.h      - Sends OpenGL calls to the gld remote graphics server.
    root/lib/libc.h    - The basic library calls that most applications expect.
    root/lib/mem.h     - malloc/free (currently under development.)
    root/lib/net.h     - Socket calls (currently TCP localhost connections only.)
    root/lib/u.h       - Instruction set and system call enumeration.

    root/usr/      - Directory for user files.  Starting directory for shell.
    root/usr/emhello.c  - Supervisor mode hello world.  Run with em.c.  In the OS, try:
                            c -o emhello emhello.c
                            em emhello

    root/usr/euhello.c - User mode hello world.  Run directly or with eu.c.  In the OS, try:
                            ./euhello
                            c -o euhello euhello.c
                            eu euhello

    root/usr/hello.c   - A more portable hello world.
    root/usr/prseg.c   - Prints out interesting memory addresses.  In the OS, try:
                            ./prseg
                            c prseg.c
                            c /bin/c.c prseg.c
                            c /bin/c.c /bin/c.c prseg.c
                            c -o prseg prseg.c
                            ./prseg

    root/usr/os/*      - Demonstrate some techniques that the OS uses.  In the OS, try:
                            c -o os0 os0.c
                            em os0

    root/usr/demo/*        - Graphical demos (most require gld.exe to be running, see above.)
    root/use/demo/calc.c   - Scientific calculator
    root/use/demo/gears.c  - OpenGL 3-D gear wheels
    root/usr/demo/sdk.c    - Curses SUDOKU solver (requires ANSI for Windows, or launch within /bin/vt).

Tutorial Walkthru:

Here are some step-by-step instructions to get started.  The prompt of "Win# >" will be used
to indicate you are at the Host Command Prompt, and "Win# $" will be used when you are inside
the OS shell.  Launch a Windows Command Prompt (or xterm) where you unpacked the files:

   (prompt)  (type this)       (commentary)

    Win1 >   cleanup          - start from a clean slate
    Win1 >   bhello           - you should see "Hello world." messages if everything is OK
    Win1 >   bos              - you should see some stripes of 111's and 000's
    Win1 >   recurse          - you should see "Hello world." running in nested emulation

    Win1 >   cleanup          - start again from a clean slate
    Win1 >   boot             - this will boot into the OS (you should see Welcome! and $ prompt)
    Win1 $   pwd              - print working directory
    Win1 $   ls               - list files
    Win1 $   ls /bin          - list command files
    Win1 $   shd &            - run a shell daemon as a background process

Launch another Command Prompt:

    Win2 >   btools           - builds srv.exe and term.exe
    Win2 >   term             - connect to the shd shell daemon (you should see a $ prompt)
    Win2 $   ls               - try some stuff in both windows...
    Win2 $   cat /bin/cat.c
    Win2 $   man cat

Launch a third Command Prompt:

    Win3 >   gld -v           - listen for incoming graphics connections (verbose mode)

Back in the first window:

    Win1 $   vt &             - run a Xterm-like client session in the background

    VT $     cd /usr/demo     - do this in the VT window
    VT $     ./sdk            - run a sudoku solver (type q to quit)

    Win1 $   halt             - halts the system (can be run from any $ prompt)
                                ** NOTE: All changes to the RAM file system are lost! **

    Win1 $   reboot           - get back into the system
    Win1 $   cd /usr/demo
    Win1 $   ./calc &         - GUI applications!

    Win1 $   ftpd &           - run an FTP server as a background process

From window 2:

    Win2 >   ftp 127.0.0.1    - FTP into the system (hit enter at User and Password)
    Win2 ftp> ls
    Win2 ftp> bye             - WinSCP works even better

For the grand finale, lets run the OS inside itself using self-emulation:

    Win1 $   halt
    Win1 >   reboot
    Win1 $   cd /etc
    Win1 $   ls                    - note the OS (os) and small file system (sfs.img)
    Win1 $   em -f sfs.img os      - enter the Matrix.  you should see Welcome! and $
    Win1 $   ls                    - it's a bit slower as you can imagine
    Win1 $   halt
    Win1 $   halt

That's all for now.

To-Do List:

    Code cleanup and fixes for XXX items.
    Fix occasional CPU exception on pipe read/write.
    Implement networking emulation correctly.
    Better timer & time handling (don't slam the CPU while running.)
    Signal handling (ctrl-C, etc.)
    Complete demand paging code (page evicting, copy on write, mmap, etc.)
    Virtual file system layer (mount/unmount, remote/user-defined file systems, etc.)
    Applications (kernel instrumentation, explorer, paint, games, little languages, etc.)
    Regression test suite.

Credits:

    The operating system kernel was ported from xv6.
    Many commands in root/bin and root/etc also originated from xv6.
    The OS and commands have been substantially modified, adapted and extended.
    xv6 is copyright 2006 Frans Kaashoek, Robert Morris, and Russ Cox.
    
    The emulator virtual memory implementation is a variation of the technique used
    in the Javascript PC Emulator written by Fabrice Bellard.
    
    Code based on other sources includes acknowledgment within each file.

    All other original code is Copyright 2014 Robert Swierczek.

I welcome your comments, suggestions, bug fixes, and improvements.  This is the first
release of the code, and much is rough and unfinished.

Robert Swierczek
Email: rswier AT acm DOT org
October 2014
