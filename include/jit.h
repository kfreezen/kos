#ifndef JIT_H
#define JIT_H

#include <KOSTypes.h>

#define INST_RETURN 			0x0000
#define INST_PUSH_IMM32			0x0001
#define INST_SYSCALL			0x0002
#define INST_ADD_SP_IMM8		0x0003
#define INST_PUSH_ADDR			0x0004
#define INST_SYSCALLNA			0x0005 // This instruction is deprecated. use INST_SYSCALL instead.
#define INST_NOP				0x9090

#define X86_PUSH_IMM32				0x68
#define X86_ADD_REG_IMM32			0x81
#define X86_ADD_REG_IMM8			0x83
#define X86_MOV_REG_IMM32			0xB8
#define X86_ADD_REG32				0xC0
#define X86_NEAR_RETURN 			0xC3
#define X86_NEAR_CALL_ABS_REG		0xD0
#define X86_NEAR_CALL_ABS			0xFF

#define X86_EAX						0x00
#define X86_EDX						0x02
#define X86_ESP						0x04
#define X86_EBP						0x05

void jit_compile(void* program, void* out);

#endif
