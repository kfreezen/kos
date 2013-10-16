C_SRC:=$(shell find src -mindepth 1 -maxdepth 3 -name "*.c")
S_SRC:=$(shell find src -mindepth 1 -maxdepth 3 -name "*.s")
T_SRC:=$(shell find src -mindepth 1 -maxdepth 3 -name "*.t.c")
DRV_SRC:=$(shell find drv_src -mindepth 1 -maxdepth 3 -name "*.c")

HDR:=$(shell find include -mindepth 1 -maxdepth 3 -name "*.h")
T_HDR:=$(shell find . -mindepth 1 -maxdepth 3 -name "*.t.h");

C_SOURCES:=$(patsubst %.c, %.o, $(C_SRC))
S_SOURCES:=$(patsubst %.s, %.o, $(S_SRC))
DRV_SOURCES:=$(patsubst %.c, %.ko, $(DRV_SRC))

SOURCES:=$(S_SOURCES) $(C_SOURCES)

CC:=@gcc
CFLAGS:=-I include -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Werror
AS:=@nasm
ASFLAGS:=-felf32
LD:=@ld
LDFLAGS:=-melf_i386 -Tlink.ld

all: $(SOURCES) $(HDR) link
	
link:
	$(LD) $(LDFLAGS) -o kernel $(SOURCES)

drivers:  $(DRV_SOURCES)

%.ko:  %.c
	$(CC) $(CFLAGS) -r -o $@ $<
	
clean:
	-@rm $(SOURCES) kernel
