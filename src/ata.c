#include <device.h>
#include <ata.h>
#include <pci.h>
#include <common/arraylist.h>
#include <print.h>
#include <kheap.h>
#include <isr.h>
#include <pic.h>

#define ATA_DEBUG

UInt32 port_base[2];
UInt32 secondary_port_base;

UInt32 dev_ctrl[2];
UInt32 secondary_dev_ctrl;

UInt32 last_drive_selected[2];