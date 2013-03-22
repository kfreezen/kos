#!/bin/bash

wc -l `find src -mindepth 1 -maxdepth 3 -name "*.s"` `find src -mindepth 1 -maxdepth 3 -name "*.c"` `find include -mindepth 1 -maxdepth 3 -name "*.h"`

sudo mount /dev/loop0 /media/loop0

./make-initrd

sudo cp kernel /media/loop0/kernel
sudo cp initrd.img /media/loop0/initrd.img
sudo cp helloworld /media/loop0/helloworld
sudo cp kbmaps.dat /media/loop0/kbmaps.dat

sleep 0.25s
sudo umount /media/loop0

sudo qemu-system-x86_64 -soundhw sb16 -cdrom /dev/cdrom -hda kos_hdd.img -vga std -fda /dev/loop0
