#ifndef COMMON_H
#define COMMON_H

#include <KOSTypes.h>

unsigned long strtoul(const char* nptr, char** endptr, int ibase);

char* strtok(char *s, const char *delim);

void memcpy(Pointer dest, Pointer src, size_t size);

#endif
