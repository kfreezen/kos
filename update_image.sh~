#!/bin/bash

wc -l `find src -mindepth 1 -maxdepth 3 -name "*.s"` `find src -mindepth 1 -maxdepth 3 -name "*.c"` `find include -mindepth 1 -maxdepth 3 -name "*.h"`

sudo mount /dev/loop0 /media/loop0

./make-initrd `tr '\n' ' ' < initrd-files`

nasm -fbin helloworld.asm

sudo cp kernel /media/loop0/kernel
sudo cp initrd.img /media/loop0/initrd.img
sudo cp helloworld /media/loop0/hlowrld.bin
sleep 0.25s
sudo umount /media/loop0

sudo qemu-system-x86_64 -soundhw sb16,ac97 -fda /dev/loop0
