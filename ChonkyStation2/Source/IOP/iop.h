// This was ported from my PS1 emulator (ChonkyStation) CPU core

#include "../Memory/memory.h";
#include <stdint.h>
#include <cstdarg>
#include <string>
#define NOMINMAX
#include <windows.h>

#define log_cpu
#define log_kernel_tty
//#undef log_cpu

class IOP
{
public:
	IOP(Memory* memptr);
	Memory* mem;
	void debug_warn(const char* fmt, ...);
	void debug_err(const char* fmt, ...);
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::string reg[32] = { "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3","$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7","$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };

	enum exceptions {
		INT = 0x0,
		BadFetchAddr = 0x4,
		BadStoreAddr = 0x5,
		SysCall = 0x8,
		Break = 0x9,
		Reserved_Instruction = 0xA,
		Overflow = 0xC
	};

	uint32_t next_instr = 0;
	uint32_t jump = 0; // jump branch delay slot
	// registers
	uint32_t pc = 0;
	uint32_t regs[32];
	struct cop0 {
		uint32_t regs[32];
	};
	cop0 COP0;
	uint32_t hi = 0;
	uint32_t lo = 0;

	void execute(uint32_t instr);
	void exception(exceptions);
	void check_dma();

	void step();
	bool delay = false;

	bool debug = false;

	std::string rom_directory;
};