#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>

using namespace std;

typedef struct {
	unsigned address;
	int str_offset;
} SymEntry;

#define HDR_MAGIC 0xFACEF00D

typedef struct {
	int magic; // 0xFACEF00D
	int symentries_offset;
	int strtab_offset;
	int entries;
} Header;

int strtabitr = 0;

int main() {
	vector<SymEntry> symEntries;
	vector<string> strings;

	do {
		string address, type, symbol;
		cin>> address >> type >> symbol;
		SymEntry sym;
		sym.address = (unsigned) strtoul(address.c_str(), NULL, 16);
		sym.str_offset = strtabitr;
		strtabitr += symbol.length() + 1;
		strings.push_back(symbol);
		symEntries.push_back(sym);
	} while(!cin.eof());

	char* strtabData = new char[strtabitr];
	vector<string>::iterator itrStrings;
	strtabitr = 0;
	for(itrStrings = strings.begin(); itrStrings < strings.end(); itrStrings++) {
		strcpy(&strtabData[strtabitr], itrStrings->c_str());

		strtabitr += itrStrings->length() + 1;
	}

	SymEntry* symEntriesBuffer = new SymEntry[symEntries.size()];

	vector<SymEntry>::iterator itrSyms;
	int i=0;
	for(itrSyms = symEntries.begin(); itrSyms < symEntries.end(); itrSyms++) {
		SymEntry symEntry = *itrSyms;
		memcpy(&symEntriesBuffer[i++], &symEntry, sizeof(SymEntry));
	}

	Header header;
	header.magic = HDR_MAGIC;
	header.strtab_offset = sizeof(Header);
	header.symentries_offset = sizeof(Header) + strtabitr;
	header.entries = symEntries.size();
	
	cout.write((const char*) &header, sizeof(Header));
	cout.write(strtabData, strtabitr);
	cout.write((const char*) symEntriesBuffer, symEntries.size()*sizeof(SymEntry));
	cout.flush();

	delete strtabData;
	delete symEntriesBuffer;
}