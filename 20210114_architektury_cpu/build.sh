#/bin/bash

PREFIXES="aarch64-linux-gnu- arm-linux-gnueabihf- $HOME/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf- /usr/bin/"

for p in $PREFIXES; do
    ${p}gcc main.c -o .obj.o
    ${p}objdump -d .obj.o > dis-$(basename $p).txt
done
