#!/bin/sh

gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o file.o file.c
gcc -c -m64 -nostdlib -nostartfiles -nodefaultlibs -fomit-frame-pointer -mno-red-zone -o libBareMetal.o libBareMetal.c
ld -T app.ld -o file.app file.o libBareMetal.o


#gcc -I newlib-2.1.0/newlib/libc/include/ -c file.c -o file.o
#gcc -c -nostdlib -nostartfiles -nodefaultlibs libBareMetal.c -o libBareMetal.o
#ld -T app.ld -o file.app crt0.o file.o libBareMetal.o libc.a

