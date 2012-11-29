#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <KOSTypes.h>

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400

struct multiboot
{
   UInt32 flags;
   UInt32 mem_lower;
   UInt32 mem_upper;
   UInt32 boot_device;
   UInt32 cmdline;
   UInt32 mods_count;
   UInt32 mods_addr;
   UInt32 num;
   UInt32 size;
   UInt32 addr;
   UInt32 shndx;
   UInt32 mmap_length;
   UInt32 mmap_addr;
   UInt32 drives_length;
   UInt32 drives_addr;
   UInt32 config_table;
   UInt32 boot_loader_name;
   UInt32 apm_table;
   UInt32 vbe_control_info;
   UInt32 vbe_mode_info;
   UInt32 vbe_mode;
   UInt32 vbe_interface_seg;
   UInt32 vbe_interface_off;
   UInt32 vbe_interface_len;
}  __attribute__((packed));

typedef struct multiboot MultibootHeader;

#endif
