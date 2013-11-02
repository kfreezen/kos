#!/bin/bash

#wc -l `find src -mindepth 1 -maxdepth 3 -name "*.s"` `find src -mindepth 1 -maxdepth 3 -name "*.c"` `find include -mindepth 1 -maxdepth 3 -name "*.h"`

sudo mount /dev/loop0 /media/loop0

./make-initrd

nm kernel | grep -f kernel.sym | ./tmtb > kernel.map

sudo cp kernel /media/loop0/kernel
sudo cp kernel.map /media/loop0/kernel.map
sudo cp initrd.img /media/loop0/initrd.img
sudo cp kbmaps.dat /media/loop0/kbmaps.dat
sudo cp init.script /media/loop0/init.script

make drivers
sudo mkdir /media/loop0/drivers
sudo cp drivers/*.ko /media/loop0/drivers/

sleep 0.25s
sudo umount /media/loop0

qemu-system-x86_64 -usb -usbdevice mouse -soundhw sb16 -vga std -hda hdd.img -monitor stdio
