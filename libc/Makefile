C_SRC:=$(shell find src -mindepth 1 -maxdepth 3 -name "*.c")
S_SRC:=$(shell find src -mindepth 1 -maxdepth 3 -name "*.s")
C_SOURCES:=$(patsubst %.c, %.o, $(C_SRC))
S_SOURCES:=$(patsubst %.s, %.o, $(S_SRC))

SOURCES:=$(S_SOURCES) $(C_SOURCES)

CC:=@gcc
CFLAGS:=-I include -m32 -nostdlib -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
AS:=@nasm
ASFLAGS:=-felf32
AR:=ar
ARFLAGS:=rcs

all: $(SOURCES) link
	
link:
	$(AR) $(ARFLAGS) libc.a $(SOURCES)
	
