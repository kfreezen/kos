#!/bin/bash

wc -l `find src -mindepth 1 -maxdepth 3 -name "*.s"` `find src -mindepth 1 -maxdepth 3 -name "*.c"` `find include -mindepth 1 -maxdepth 3 -name "*.h"`

sudo mount /dev/loop0 /media/loop0

./make-initrd `tr '\n' ' ' < initrd-files`

sudo cp kernel /media/loop0/kernel
sudo cp initrd.img /media/loop0/initrd.img
sudo cp test1.txt /media/loop0/test1.txt
sleep 0.25s
sudo umount /media/loop0

sudo qemu-system-x86_64 -fda /dev/loop0
