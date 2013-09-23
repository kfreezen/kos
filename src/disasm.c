#include <disasm.h>
#include <print.h>

#define OP_AL 0x0
#define OP_CL 0x1
#define OP_DL 0x2
#define OP_BL 0x3
#define OP_AH 0x4
#define OP_CH 0x5
#define OP_DH 0x6
#define OP_BH 0x7

#define OP_AX 0x0
#define OP_CX 0x1
#define OP_DX 0x2
#define OP_BX 0x3
#define OP_SP 0x4
#define OP_BP 0x5
#define OP_SI 0x6
#define OP_DI 0x7

#define OP_ADD_RM8_R8 0x00

#define OP_EXTEND_INST0 0x0f

#define OP_AND_EAX_IMM 0x25

#define OP_XOR_REG32_REG32 0x31

#define OP_PUSH_REG 0x50
#define OP_POP_REG 0x58

#define OP_ALU_OR_REG 0xC8

#define OP_ALU_RM8_IMM8 0x80
#define OP_TEST_RR 0x85

#define OP_MOVE_REG32_MEM32 0x8B

#define OP_NOP 0x90

#define OP_MOVE_EAX_MEM 0xA1
#define OP_MOVE_MEM_EAX 0xA3
#define OP_MOVE_REG_IMM8 0xB0
#define OP_MOVE_REG_IMM16 0xB8
#define OP_MOVE_REG_IMM32 0xB8

#define OP_RET_IMM 0xC2
#define OP_RET 0xC3

#define OP_LRET_IMM 0xCA
#define OP_LRET 0xCB

#define OP_LOOPNE 0xE0

#define OP_CALL_REL_IMM 0xE8

#define OP_JMP_SHORT 0xEB

#define OP_LOCK 0xF0

#define OP_CLI 0xFA
#define OP_STI 0xFB

#define STR_EXT "e"
#define STR_AX "ax"
#define STR_CX "cx"
#define STR_DX "dx"
#define STR_BX "bx"
#define STR_SP "sp"
#define STR_BP "bp"
#define STR_SI "si"
#define STR_DI "di"
#define STR_AL "al"
#define STR_CL "cl"
#define STR_DL "dl"
#define STR_BL "bl"
#define STR_AH "ah"
#define STR_CH "ch"
#define STR_DH "dh"
#define STR_BH "bh"

#define STR_EAX "eax"
#define STR_ECX "ecx"
#define STR_EDX "edx"
#define STR_EBX "ebx"
#define STR_ESP "esp"
#define STR_EBP "ebp"
#define STR_ESI "esi"
#define STR_EDI "edi"

#define STR_ADC_RM_IMM8 "adc.rm.ib"
#define STR_OR_RM8_IMM8 "or.rm.ib"

#define STR_CALL_REL_IMM "call.ri"

#define STR_ADD "add"
#define STR_AND "and"
#define STR_CLI "cli"
#define STR_JMP_SHORT "jmp short"
#define STR_LOOPNE "loopne"
#define STR_LRET "lret"
#define STR_MOVE_REG_MEM "mov"
#define STR_MOVE_MEM_REG "mov"
#define STR_MOVE_IMM8 "mov"
#define STR_MOVE_IMM16 "mov"
#define STR_MOVE_REG_IMM32 "mov"
#define STR_NOP "nop"
#define STR_POP "pop"
#define STR_PUSH "push"
#define STR_RET "ret"
#define STR_STI "sti"
#define STR_TEST "test"
#define STR_XOR "xor"

extern UInt32 signExtendImm8(UInt8 i);
 
char* registers[8] = {STR_AX, STR_CX, STR_DX, STR_BX, STR_SP, STR_BP, STR_SI, STR_DI};

char* eregisters[8] = {STR_EAX, STR_ECX, STR_EDX, STR_EBX, STR_ESP, STR_EBP, STR_ESI, STR_EDI};

char* lowregs[8] = {STR_AL, STR_CL, STR_DL, STR_BL, STR_AH, STR_CH, STR_DH, STR_BH};

#define REGS_8BIT 0x1

// *ptr starts at the effective address.
void PrintEffectiveAddress(Byte** _ptr, char** usedregs, int flags) {
	Byte* ptr = *_ptr;
	
	Byte attr = *(ptr);
	//kprintf("attr=%x\n", attr);
	
	UInt32 destreg = attr & 0x3F;
	destreg = destreg >> 3;
	
	UInt32 srcreg = attr & 0x7;
	int scale = 0;
	int scalereg = 0;
	
	Byte regs = attr&0x3f;
	int src, dest;
	src = regs / 8;
	dest = regs%8;
		
	if((attr&0xc0)==0xc0) {
		
		if(flags&REGS_8BIT) {
			usedregs = lowregs;
		}
		
		kprintf("%s, %s", usedregs[dest], usedregs[src]);
		return;
	} else {
		kprintf("%s, [", (flags&REGS_8BIT) ? lowregs[dest] : usedregs[dest]);
	}
	
	if((srcreg&0x7)==0x4) {
		// SIB byte.
		Byte sib = *(++ptr);
		scalereg = sib / 8;
		scale = sib / 0x40;
		switch(scale) {
			case 0: scale = 1;
				break;
			case 1: scale = 2;
				break;
			case 2: scale = 4;
				break;
			case 3: scale = 8;
				break;
		}
	
		kprintf("%s*%x", (scalereg==4) ? "0" : usedregs[scalereg], scale);
	} else if(srcreg==0x5) {
		// Memory.
		UInt32 addr = *((UInt32*) ((UInt32)++ptr));
		kprintf("%x", addr);
		
		ptr+=4;
		
	}
	if(scale) {
		kprintf("+");
	}
	
	kprintf("%s", usedregs[srcreg]);

	if(attr&0x40) {
		// sbyte.
		char sbyte = *(++ptr);
		char sign;
	
		if(sbyte&0x80) {
			sign = '-';
			sbyte = -sbyte;
		} else {
			sign = '+';
		}
	
		kprintf("%c%x", sign, sbyte);
	}
	
	kprintf("]");
	
	*_ptr = ptr;
}

inline UInt32 uint_peek(void* ptr) {
	return *((UInt32*) ((UInt32)ptr));
}

void DisassembleExtendedInst0(Byte** _ptr, char** usedregs);

void Disassemble(UInt32 addr, int instructions) {
	Byte* ptr = (Byte*) addr;
	
	int i;
	
	char** usedregs = eregisters;
	
	//char* instStr;
	for(i=0; i<instructions;) {
		kprintf("%x: ", ptr);
		
		if(*ptr==OP_EXTEND_INST0) {
			ptr++;
			DisassembleExtendedInst0(&ptr, usedregs);
			i++;
			continue;	
		}
		
		if(*ptr==OP_LOCK) {
			kprintf("lock ");
			ptr++;
		}
	
		if( ((*ptr)&0xF8) == OP_MOVE_REG_IMM32) {
			kprintf("%s %s, %x\n", STR_MOVE_REG_IMM32, usedregs[(*ptr)&0x7], uint_peek(ptr+1));
			ptr++;
			
			ptr+=4;
			i++;
		} else if(*ptr==OP_NOP) {
			kprintf("%s\n", STR_NOP);
			ptr++;
			i++;
		} else if(*ptr==OP_JMP_SHORT) {
			UInt8 rel = *(ptr+1);
			ptr+=2;
			
			unsigned addr = (UInt32) ptr;
			
			addr += signExtendImm8(rel);
			i++;
			
			kprintf("%s %x\n", STR_JMP_SHORT, addr);
		} else if(*ptr==OP_CLI) {
			ptr++;
			i++;
			kprintf("%s\n", STR_CLI);
		} else if(*ptr==OP_STI) {
			ptr++;
			i++;
			kprintf("%s\n", STR_STI);
		} else if(*ptr==OP_ALU_RM8_IMM8) {
			// FIXME:  This needs some major reworking.
			Byte attr = *(++ptr);
			UInt32 address;
			if(attr == 0x15) {
				address = *((UInt32*) ((UInt32)ptr+1));
				ptr+= 5;
				kprintf("%s [%x], %x\n", STR_ADC_RM_IMM8, address, *ptr);
			} else if((attr&OP_ALU_OR_REG)==OP_ALU_OR_REG) {
				kprintf("%s %s, %x\n", STR_OR_RM8_IMM8, lowregs[attr&~OP_ALU_OR_REG], *ptr);
			} else {
				kprintf("%s attr(%x), %x\n", "alu", attr, *ptr);
			}
			
			ptr++;
			i++;
		} else if(*ptr == OP_MOVE_REG32_MEM32) {
			Byte attr = *(++ptr);
			//kprintf("attr=%x\n", attr);
			
			UInt32 destreg = attr & 0x3F;
			destreg = destreg >> 3;
			
			kprintf("%s ", STR_MOVE_REG_MEM);
			
			PrintEffectiveAddress(&ptr, usedregs, 0);
			
			kprintf("\n");
			
			ptr++;
			i++;
		} else if(*ptr == OP_MOVE_MEM_EAX) {
			UInt32 addr = *((UInt32*) ((UInt32)ptr+1));
			
			kprintf("%s eax, [%x]\n", STR_MOVE_MEM_REG, addr);
			ptr+=5;
			i++;
		} else if(*ptr == OP_CALL_REL_IMM) {
			UInt32 reladdr = uint_peek(++ptr);
			ptr+=4;
			kprintf("%s %x\n", STR_CALL_REL_IMM, reladdr+ptr);
			i++;
		} else if(*ptr==OP_MOVE_EAX_MEM) { // mov eax, [memlocation]
			UInt32 addr = uint_peek(++ptr);
			
			ptr+=4;
			i++;
			
			kprintf("%s eax, [%x]\n", STR_MOVE_REG_MEM, addr);
		} else if(*ptr==OP_TEST_RR) {
			//Byte attr = *(++ptr);
			
			kprintf("%s ", STR_TEST);
			PrintEffectiveAddress(&ptr, usedregs, 0);
			
			ptr++;
			i++;
			
		} else if(*ptr==OP_LOOPNE) {
			char sbyte = *(++ptr);
			
			int sub;
			sub = (sbyte&0x80) ? 1 : 0;
			sbyte = (sbyte&0x80) ? -sbyte : sbyte;
			
			UInt32 addr = (UInt32) ++ptr;
			
			addr = (sub) ? addr-sbyte : addr+sbyte;
			
			kprintf("%s %x\n", STR_LOOPNE, addr);
			
			i++;
		} else if(*ptr==OP_AND_EAX_IMM) {
			UInt32 imm = uint_peek(++ptr);
			
			ptr+=4;
			i++;
			
			kprintf("%s eax, %x\n", STR_AND, imm);
		} else if(*ptr==OP_RET_IMM || *ptr==OP_RET) {
			char c = (*ptr==OP_RET_IMM) ? ' ' : '\n';
			
			kprintf("%s%c", STR_RET, c);
			
			if(*ptr==OP_RET_IMM) {
				kprintf("%x\n", *(++ptr));
			}
			
			ptr++;
			i++;
		} else if(*ptr==OP_LRET_IMM || *ptr==OP_LRET) {
			char c = (*ptr==OP_LRET_IMM) ? ' ' : '\n';
			
			kprintf("%s%c", STR_LRET, c);
			
			if(*ptr==OP_LRET_IMM) {
				kprintf("%x\n", *(++ptr));
			}
			
			ptr++;
			i++;
		} else if((*ptr&OP_PUSH_REG)==OP_PUSH_REG) {
			if((*ptr&OP_POP_REG)==OP_POP_REG) {
				kprintf("%s %s\n", STR_POP, usedregs[*ptr&0x7]);
			} else {
				kprintf("%s %s\n", STR_PUSH, usedregs[*ptr&0x7]);
			}
			
			ptr++;
			i++;
		} else if(*ptr == OP_XOR_REG32_REG32) {
			//Byte attr = *(++ptr);
			
			PrintEffectiveAddress(&ptr, usedregs, 0);
			
			PutChar('\n');
			
			i++;
		} else if(*ptr == OP_ADD_RM8_R8) {
			//Byte attr = *(++ptr);
			//Byte regs = attr&0x3f;
			
			kprintf("%s ", STR_ADD);
			PrintEffectiveAddress(&ptr, usedregs, REGS_8BIT);
			
			PutChar('\n');
			
			i++;
		}
		
		else {
			kprintf("%s:%x\n", "<error>", *ptr);
			i++;
			ptr++;
		}
	}
	
}

#define OP_LTR 0x00

#define STR_LTR "ltr"

void DisassembleExtendedInst0(Byte** _ptr, char** usedregs) {
	Byte* ptr = *_ptr;
	
	if(*ptr==OP_LTR) {
		int reg = *(++ptr) & ~0xd8;
		kprintf("%s %s\n", STR_LTR, registers[reg]);
	
		ptr++;
	}
	
	else {
		kprintf("%s:%x,%x\n", "<error>", *ptr, *(ptr+1));
		
		ptr+=2;
	}
	
	*_ptr = ptr;
}
