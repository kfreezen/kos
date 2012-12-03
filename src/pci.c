#include <pci.h>
#include <common/arraylist.h>
#include <kheap.h>
#include <print.h>

ArrayList TYPE(PCIDevice*)* pciDevices = NULL;

UInt32 pciConfigReadLong(UInt16 bus, UInt16 slot, UInt16 func, UInt16 offset) {
	UInt32 address;
	UInt32 lbus = (UInt32)bus&0xFF;
	UInt32 lslot = (UInt32)slot;
	UInt32 lfunc = (UInt32)func;
	
	address = (UInt32)((lbus<<16) | (lslot<<11) | (lfunc<<8) | (offset&0xfc) | 0x80000000);
	outl(PCI_CONFIG_ADDR, address);
	
	return inl(PCI_CONFIG_DATA);
}

void pciConfigWriteLong(UInt16 bus, UInt16 slot, UInt16 func, UInt16 offset, UInt32 write) {
	UInt32 address;
	UInt32 lbus = (UInt32)bus&0xFF;
	UInt32 lslot = (UInt32)slot;
	UInt32 lfunc = (UInt32)func;
	
	address = (UInt32)((lbus<<16) | (lslot<<11) | (lfunc<<8) | (offset&0xFC) | 0x80000000);
	outl(PCI_CONFIG_ADDR, address);
	outl(PCI_CONFIG_DATA, write);
}

inline UInt16 getVendorID(UInt16 bus, UInt16 slot, UInt16 function) {
	return pciConfigReadLong(bus, slot, function, 0)&0xFFFF;
}

inline UInt16 getHeaderType(UInt16 bus, UInt16 slot, UInt16 function ) {
	return (pciConfigReadLong(bus, slot, function, 0xc))&0xFF;
}

inline UInt16 getBaseClass(UInt16 bus, UInt16 slot) {
	return (pciConfigReadLong(bus, slot, 0, 0x8)>>24)&0xFF;
}

inline UInt16 getSubClass(UInt16 bus, UInt16 slot) {
	return (pciConfigReadLong(bus, slot, 0, 0x8)>>16)&0xFF;
}

inline UInt16 getSecondaryBus(UInt16 bus, UInt16 slot) {
	return (pciConfigReadLong(bus, slot, 0, 0x18)>>8)&0xFF;
}

inline UInt16 getDeviceID(UInt16 bus, UInt16 slot) {
	return pciConfigReadLong(bus, slot, 0, 0)>>16;
}

void AddPCIDevice(UInt8 bus, UInt8 device, UInt8 functionsNum) {
	if(pciDevices == NULL) {
		pciDevices = ALCreate();
	}
	
	PCIDevice* dev = (PCIDevice*) kalloc(sizeof(PCIDevice));
	dev->bus = bus;
	dev->device = device;
	dev->functionsNum = functionsNum;
	dev->vendorID = getVendorID(bus, device, 0);
	dev->deviceID = getDeviceID(bus, device);
	ALAdd(pciDevices, dev);
}

PCIDevice* GetPCIDevice(UInt16 vendorID, UInt16 deviceID) {
	ALIterator* itr = ALGetItr(pciDevices);
	
	while(ALItrHasNext(itr)) {
		PCIDevice* dev = (PCIDevice*) ALItrNext(itr);
		if(dev->vendorID == vendorID && dev->deviceID == deviceID) {
			ALFreeIterator(itr);
			return dev;
		}
	}
	
	return NULL;
}

void checkBus(UInt8 bus) {
	UInt8 device;
	
	for(device = 0; device<32; device++) {
		checkDevice(bus, device);
	}
}

void checkFunction(UInt8 bus, UInt8 device, UInt8 function) {
	UInt8 baseClass, subClass, secondaryBus;
	
	baseClass = getBaseClass(bus, device);
	subClass = getSubClass(bus, device);
	
	if( (baseClass==0x06)&&(subClass==0x04) ) {
		secondaryBus = getSecondaryBus(bus, device);
		checkBus(secondaryBus);
	}
}

void checkDevice(UInt8 bus, UInt8 device) {
	UInt8 function = 0;
	
	UInt8 vendorID = getVendorID(bus, device, function);
	if(vendorID==0xFFFF) {
		return;
	}
	
	checkFunction(bus, device, function);
	UInt8 headerType = getHeaderType(bus, device, function);
	if(headerType&PCI_MULTIFUNCTION) {
		for(function = 1; function < 8; function++) {
			if(getVendorID(bus, device, function)==0xFFFF) {
				break;
			}
			
			checkFunction(bus, device, function);
		}
	}
	
	if(getVendorID(bus, device, 0)==0xFFFF) {
		return;
	}
	
	AddPCIDevice(bus, device, function+1);
}

void checkAllBuses(void) {
	UInt8 function;
	UInt8 bus;
	
	UInt8 headerType = getHeaderType(0,0,0);
	if( (headerType&PCI_MULTIFUNCTION)==0) {
		checkBus(0);
	} else {
		for(function = 0; function < 8; function++) {
			if(getVendorID(0,0, function) == 0xFFFF) {
				break;
			}
			
			bus = function;
			checkBus(bus);
		}
	}
}

void DumpPCIDeviceData() {
	ALIterator* itr = ALGetItr(pciDevices);
	
	while(ALItrHasNext(itr)) {
		PCIDevice* device = (PCIDevice*) ALItrNext(itr);
		kprintf("vendor=%x, device=%x, functionsNum=%x\n", device->vendorID, device->deviceID, device->functionsNum);
	}
	
	ALFreeIterator(itr);
}
