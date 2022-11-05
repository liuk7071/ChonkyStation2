#pragma once
#include "../common.h"
#include "../Memory/memory.h";

//#define BIOS_HLE

class EE {
public:
	EE(Memory* memptr);
	Memory* mem;

	void Step();
	bool traceb = false;

	// Registers
	u128 gprs[32];
	u128 lo, hi;
	u128 cop0r[31];
	std::string gpr[32] = { "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3","$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7","$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };
	u32 pc = 0x1fc00000;
	u32 old_pc; // Mostly used for printing the pc of instructions in delay slots
	std::optional<u32> branch_pc = std::nullopt;

	// Exceptions
	enum Exceptions {
		Interrupt = 0x00,
		ExSYSCALL = 0x08
	};
	void Exception(unsigned int exception, bool int1 = false);

	// Instructions
	union Instruction {
		u32 raw;
		BitField<0, 16, u32> imm;
		BitField<0, 26, u32> jump_imm;
		BitField<6, 5, u32>  sa;
		BitField<11, 5, u32> rd;
		BitField<16, 5, u32> rt;
		BitField<21, 5, u32> rs;
	};
	void Execute(Instruction instruction);
	enum Instructions {
		SPECIAL = 0x00,
		REGIMM  = 0x01,
		J       = 0x02,
		JAL     = 0x03,
		BEQ     = 0x04,
		BNE     = 0x05,
		BLEZ    = 0x06,
		BGTZ    = 0x07,
		ADDI    = 0x08,
		ADDIU   = 0x09,
		SLTI    = 0x0a,
		SLTIU   = 0x0b,
		ANDI    = 0x0c,
		ORI     = 0x0d,
		XORI    = 0x0e,
		LUI     = 0x0f,
		COP0    = 0x10,
		COP1    = 0x11,
		COP2    = 0x12,
		BEQL    = 0x14,
		BNEL    = 0x15,
		BLEZL   = 0x16,
		DADDIU  = 0x19,
		LDL     = 0x1a,
		LDR     = 0x1b,
		MMI     = 0x1c,
		LQ      = 0x1e,
		SQ      = 0x1f,
		LB      = 0x20,
		LH      = 0x21,
		LW      = 0x23,
		LBU     = 0x24,
		LHU     = 0x25,
		LWU     = 0x27,
		SB      = 0x28,
		SH      = 0x29,
		SW      = 0x2b,
		SDL     = 0x2c,
		SDR     = 0x2d,
		CACHE   = 0x2f,
		LD      = 0x37,
		LWC1    = 0x31,
		SWC1    = 0x39,
		SD      = 0x3f
	};
	enum SPECIAL {
		SLL     = 0x00,
		SRL     = 0x02,
		SRA     = 0x03,
		SLLV    = 0x04,
		SRLV    = 0x06,
		SRAV    = 0x07,
		JR      = 0x08,
		JALR    = 0x09,
		MOVZ    = 0x0a,
		MOVN    = 0x0b,
		SYSCALL = 0x0c,
		SYNC    = 0x0f,
		MFHI    = 0x10,
		MTHI    = 0x11,
		MFLO    = 0x12,
		MTLO    = 0x13,
		DSLLV   = 0x14,
		DSRAV	= 0x17,
		MULT    = 0x18,
		DIV     = 0x1a,
		DIVU    = 0x1b,
		ADD     = 0x20,
		ADDU    = 0x21,
		SUB     = 0x22,
		SUBU    = 0x23,
		AND     = 0x24,
		OR      = 0x25,
		XOR     = 0x26,
		NOR		= 0x27,
		MTSA    = 0x29,
		SLT     = 0x2a,
		SLTU    = 0x2b,
		DADDU   = 0x2d,
		DSUB    = 0x2e,
		DSUBU   = 0x2f,
		MFSA    = 0x28,
		TGE     = 0x30,
		DSLL    = 0x38,
		DSRL    = 0x3a,
		DSLL32  = 0x3c,
		DSRL32  = 0x3e,
		DSRA32  = 0x3f
	};
	enum REGIMM {
		BLTZ  = 0x00,
		BGEZ  = 0x01,
		BLTZL = 0x02,
		BGEZL = 0x03,
		MTSAH = 0x19
	};
	enum MMI {
		PLZCW = 0x04,
		MMI0  = 0x08,
		MMI2  = 0x09,
		MFHI1 = 0x10,
		MTHI1 = 0x11,
		MFLO1 = 0x12,
		MTLO1 = 0x13,
		MULT1 = 0x18,
		DIVU1 = 0x1b,
		MMI1  = 0x28,
		MMI3  = 0x29
	};
	enum MMI0 {
		PADDSH = 0x17,
		PPAC5  = 0x1f
	};
	enum MMI1 {
		PADDUW = 0x10,
		PCEQB  = 0x0a
	};
	enum MMI2 {
		PCPYLD = 0x0e,
	};
	enum MMI3 {
		PCPYUD = 0x0e,
		POR    = 0x12,
		PCPYH  = 0x1b
	};
	enum COP0 {
		MFC0 = 0x00,
		MTC0 = 0x04,
		TLB  = 0x10
	};
	enum TLB {
		TLBWI = 0x02,
		ERET  = 0x18,
		EI    = 0x38,
		DI    = 0x39
	};

	// HLE SYSCALLs
	void Syscall(u64 v1);
	void SetGsCrt(bool interlaced, int display_mode, bool frame);
	void GsPutIMR(u64 value);
	struct CustomSyscall {
		int index;
		u32 address;
	};
	std::vector<CustomSyscall> custom_syscalls;
	u64 ra_old;
	u64 pc_old;
	void SetSyscall(int index, u32 address);
	int main_thread_stack;
	void InitMainThread(u32 gp, u32 stack, int stack_size, u32 args, int root);
	void InitHeap(u32 heap, int heap_size);
	int sema_id = 0;
	void CreateSema(u32 semaparam_ptr);
	void DeleteSema(int sema_id);
	void WaitSema(int sema_id);
	void SifSetDma(u32 transfer_ptr, int len);
};