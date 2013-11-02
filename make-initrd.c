#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INITRD_MAGIC 0x12345678

struct __InitrdHeader {
  unsigned magic;
  int numFiles;
  unsigned headersOffset;
  char reserved[48];
} __attribute__((packed));

typedef struct __InitrdHeader InitrdHeader;

struct __InitrdFileHeader {
  unsigned magic;
  char name[60];
  unsigned dataOffset;
  unsigned length;
  char reserved[24];
} __attribute__((packed));

typedef struct __InitrdFileHeader InitrdFileHeader;


int main(char argc, char **argv)
{
    int nheaders = (argc-1);
    InitrdHeader header;
    InitrdFileHeader* headers = malloc(sizeof(InitrdFileHeader)*nheaders);//[64];
    printf("size of header: %d\n", sizeof(InitrdFileHeader));

    header.magic = INITRD_MAGIC;
    header.numFiles = nheaders;

    FILE *wstream = fopen("./initrd.img", "w");

    unsigned int off = sizeof(InitrdHeader);
    header.headersOffset = off;

    off = off + sizeof(InitrdFileHeader) * nheaders;

    int i;
    for(i = 0; i < nheaders; i++)
    {
       printf("writing file %s->%s at 0x%x.\n", argv[i+1], argv[i+1], off);
       strcpy(headers[i].name, argv[i+1]);
       headers[i].dataOffset = off;
       FILE *stream = fopen(argv[i+1], "r");
       if(stream == 0)
       {
         printf("Error: file not found: %s\n", argv[i+1]);
         return 1;
       }
       fseek(stream, 0, SEEK_END);
       headers[i].length = ftell(stream);
       off += headers[i].length;
       fclose(stream);
       headers[i].magic = INITRD_MAGIC;
       printf("%d\n", headers[i].length);

    }

   unsigned char *data = (unsigned char *)malloc(off);
   fwrite(&header, sizeof(InitrdHeader), 1, wstream);
   fwrite(headers, sizeof(InitrdFileHeader), nheaders, wstream);
   
   for(i = 0; i < nheaders; i++)
   {
     FILE *stream = fopen(argv[i+1], "r");
     unsigned char *buf = (unsigned char *)malloc(headers[i].length);
     fread(buf, 1, headers[i].length, stream);
     fwrite(buf, 1, headers[i].length, wstream);
     fclose(stream);
     free(buf);
   }
   
   fclose(wstream);
   free(data);
   
   return 0;
} 
