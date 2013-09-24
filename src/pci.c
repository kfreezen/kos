#include <pci.h>
#include <common/arraylist.h>
#include <kheap.h>
#include <print.h>

//#define PCI_DEBUG

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

UInt32 PCI_ReadConfigLong(PCIDevice* device, int func, int config_offset) {
	if(func < 0 && func >= device->functionsNum) {
		func = 0;
	}
	
	return pciConfigReadLong(device->bus, device->device, func, config_offset);
	
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

inline UInt16 getDeviceID(UInt16 bus, UInt16 slot, UInt16 function) {
	return pciConfigReadLong(bus, slot, function, 0)>>16;
}

void AddPCIDevice(UInt8 bus, UInt8 device, UInt8* functions, UInt8 functionsNum) {
	if(pciDevices == NULL) {
		pciDevices = ALCreate();
	}
	
	PCIDevice* dev = (PCIDevice*) kalloc(sizeof(PCIDevice));
	dev->bus = bus;
	dev->device = device;
	dev->functionsNum = functionsNum;
	dev->vendorID = getVendorID(bus, device, 0);
	dev->deviceID = getDeviceID(bus, device, 0);
	memcpy(functions, dev->availableFunctions, 8);
	
	ALAdd(pciDevices, dev);
}

PCIDevice* GetPCIDevice(UInt16 vendorID, UInt16 deviceID) {
	ALIterator* itr = ALGetItr(pciDevices);
	
	while(ALItrHasNext(itr)) {
		PCIDevice* dev = (PCIDevice*) ALItrNext(itr);
		if(dev->vendorID == vendorID && dev->deviceID == deviceID) {
			ALFreeItr(itr);
			return dev;
		}
	}
	
	ALFreeItr(itr);
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
	
	#ifdef PCI_DEBUG
	if(baseClass!=0xFF && subClass!=0xFF) {
		kprintf("baseClass=%x, subClass=%x\n", baseClass, subClass);
	}
	#endif
	
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
	UInt8 availFunctions[8];
	int functions = 1;
	if(headerType&PCI_MULTIFUNCTION) {
		availFunctions[0] = 1;
		
		for(function = 1; function < 8; function++) {
			if(getVendorID(bus, device, function)==0xFFFF) {
				availFunctions[function] = 0;
			} else {
				availFunctions[function] = 1;
				functions++;
				checkFunction(bus, device, function);
			}
		}
	}
	
	if(getVendorID(bus, device, 0)==0xFFFF) {
		return;
	}
	
	AddPCIDevice(bus, device, availFunctions, functions);
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
				
			} else {
			
				bus = function;
				checkBus(bus);
			}
		}
	}
}

void DumpPCIDeviceData() {
	ALIterator* itr = ALGetItr(pciDevices);
	
	while(ALItrHasNext(itr)) {
		PCIDevice* device = (PCIDevice*) ALItrNext(itr);
		kprintf("vendor=%x, device=%x, functionsNum=%x\n", device->vendorID, device->deviceID, device->functionsNum);
	}
	
	ALFreeItr(itr);
}

ArrayList* FindPCIDevicesWithClassCode(int code) {

	ArrayList* functions = ALCreate();
	
	ALIterator* itr = ALGetItr(pciDevices);
	while(ALItrHasNext(itr)) {
		PCIDevice* device = (PCIDevice*) ALItrNext(itr);
		int func;
		
		for(func=0; func<8; func++) {
			if(device->availableFunctions[func]) {
				PCIFunction* function = kalloc(sizeof(PCIFunction));
				function->deviceID = getDeviceID(device->bus, device->device, func);
				function->vendorID = getVendorID(device->bus, device->device, func);
				function->parentDev = device;
				function->parentFunction = func;
				
				UInt32 classCode = PCI_ReadConfigLong(device, func, 0x08);
				classCode = (classCode&0xFF000000) >> 24;
				
				if(classCode==code) {
					ALAdd(functions, function);
					
					#ifdef PCI_DEBUG
					kprintf("pci_dev = %x, pci_vendor = %x, func = %x\n", function->deviceID, function->vendorID, function->parentFunction);
					#endif
				}
			}
		}
	}
	
	ALFreeItr(itr);
	
	return functions;
}
