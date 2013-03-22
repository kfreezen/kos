#ifndef PCI_H
#define PCI_H

#include <KOSTypes.h>
#include <common/arraylist.h>

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_MULTIFUNCTION 0x80
#define PCI_BIOS_ID 0x49435024

#define CLASS_MASS_STORAGE_CTRL 0x01

void checkBus(UInt8 bus);
void checkFunction(UInt8 bus, UInt8 device, UInt8 function);
void checkDevice(UInt8 bus, UInt8 device);
void checkAllBuses(void);

void DumpPCIDeviceData();

typedef struct PCIDevice {
	UInt8 bus;
	UInt8 device;
	UInt16 vendorID;
	UInt16 deviceID;
	UInt8 availableFunctions[8];
	UInt8 functionsNum;
} PCIDevice;

typedef struct PCIFunction {
	PCIDevice* parentDev;
	UInt16 vendorID;
	UInt16 deviceID;
	UInt8 parentFunction;
} PCIFunction;

PCIDevice* GetPCIDevice(UInt16 vendorID,UInt16 deviceID);

UInt32 pciConfigReadLong(UInt16 bus, UInt16 slot, UInt16 func, UInt16 offset);

ArrayList* FindPCIDevicesWithClassCode(int code);
UInt32 PCI_ReadConfigLong(PCIDevice* device, int func, int config_offset);
#endif
