#!/bin/bash

#wc -l `find src -mindepth 1 -maxdepth 3 -name "*.s"` `find src -mindepth 1 -maxdepth 3 -name "*.c"` `find include -mindepth 1 -maxdepth 3 -name "*.h"`

udisksctl mount -b /dev/loop0

./make-initrd `cat initrd-files`

nm kernel | grep -f kernel.sym | ./tmtb > kernel.map

cp kernel /media/kent/GRUB/kernel
cp kernel.map /media/kent/GRUB/kernel.map
cp initrd.img /media/kent/GRUB/initrd.img
cp kbmaps.dat /media/kent/GRUB/kbmaps.dat
cp init.script /media/kent/GRUB/init.script

cp proj/helloworld /media/kent/GRUB/helloworld

make drivers
mkdir /media/kent/GRUB/drivers
cp drivers/*.ko /media/kent/GRUB/drivers/

sleep 0.25s
umount /media/kent/GRUB

qemu-system-x86_64 -usb -usbdevice mouse -soundhw sb16 -vga std -fda grub_disk.img -monitor stdio
