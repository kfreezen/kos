#include <jit.h>
#include <emulator_syscalls.h>

extern unsigned getDWord(void* mem);
extern unsigned short getWord(void* mem);

extern void setDWord(void* mem, unsigned value);

extern unsigned short bswap16(unsigned short s);

void jit_compile(void* program, void* out) {
	unsigned pc=0;
	unsigned out_pc = 0;
	
	//kprintf("in=%x\nout=%x\n", program, out);
	for(;;) {
		unsigned short inst = bswap16(getWord(program+pc));
		pc+=2;
		if(inst==INST_RETURN) {
			unsigned char* ptr = out+out_pc;
			*ptr = X86_NEAR_RETURN;
			return;
		} else if(inst==INST_PUSH_IMM32) {
			unsigned imm = getDWord(program+pc);
			pc+=4;
			*((Byte*) out+out_pc++) = X86_PUSH_IMM32;
			
			setDWord((void*) out+out_pc, (unsigned) imm);
			out_pc+=4;
		} else if(inst==INST_SYSCALL) {
			// push imm
			unsigned n = *((Byte*) program+pc++);
			*((Byte*) out+out_pc++) = X86_PUSH_IMM32;
			setDWord((void*) out+out_pc, n);
			out_pc+=4;
			// mov eax, syscall
			*((Byte*) out+out_pc++) = X86_MOV_REG_IMM32;
			setDWord((void*) out+out_pc, (unsigned) &syscall);
			out_pc+=4;
			// call eax
			*((Byte*) out+out_pc++) = X86_NEAR_CALL_ABS;
			*((Byte*) out+out_pc++) = X86_NEAR_CALL_ABS_REG | X86_EAX;
			// add esp, 4
			*((Byte*) out+out_pc++) = X86_ADD_REG_IMM8;
			*((Byte*) out+out_pc++) = X86_ADD_REG32 | X86_ESP;
			*((Byte*) out+out_pc++) = 0x04;
		} else if(inst==INST_ADD_SP_IMM8) {
			// ADD ESP, IMM8
			*((Byte*) out+out_pc++) = X86_ADD_REG_IMM8;
			*((Byte*) out+out_pc++) = X86_ADD_REG32 | X86_ESP;
			*((Byte*) out+out_pc++) = *((Byte*) program+pc++);
		} else if(inst==INST_PUSH_ADDR) {
			unsigned addr = getDWord((void*) program+pc);
			pc+=4;
			// PUSH program+imm32
			*((Byte*) out+out_pc++) = X86_PUSH_IMM32;
			setDWord((void*) out+out_pc, (unsigned) (program+addr));
			out_pc+=4;
		} else if(inst==INST_SYSCALLNA) {
			// push imm
			unsigned n = *((Byte*) program+pc++);
			*((Byte*) out+out_pc++) = X86_PUSH_IMM32;
			setDWord((void*) out+out_pc, n);
			out_pc+=4;
			// mov eax, syscall
			*((Byte*) out+out_pc++) = X86_MOV_REG_IMM32;
			setDWord((void*) out+out_pc, (unsigned) &syscallna);
			out_pc+=4;
			// call eax
			*((Byte*) out+out_pc++) = X86_NEAR_CALL_ABS;
			*((Byte*) out+out_pc++) = X86_NEAR_CALL_ABS_REG | X86_EAX;
			// add esp, 4
			*((Byte*) out+out_pc++) = X86_ADD_REG_IMM8;
			*((Byte*) out+out_pc++) = X86_ADD_REG32 | X86_ESP;
			*((Byte*) out+out_pc++) = 0x04;
		}
	}
	
}
