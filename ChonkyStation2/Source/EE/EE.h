#pragma once
#include "../common.h"
#include "../Memory/memory.h";

class EE {
public:
	EE(Memory* memptr);
	Memory* mem;

	void Step();

	// Registers
	u128 gprs[32];
	u128 lo, hi;
	std::string gpr[32] = { "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3","$t4", "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7","$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra" };
	u32 pc = 0x1fc00000;
	u32 old_pc; // Mostly used for printing the pc of instructions in delay slots
	std::optional<u32> branch_pc = std::nullopt;

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
		ADDIU   = 0x09,
		SLTI    = 0x0a,
		SLTIU   = 0x0b,
		ANDI    = 0x0c,
		ORI     = 0x0d,
		LUI     = 0x0f,
		COP0    = 0x10,
		BEQL    = 0x14,
		BNEL    = 0x15,
		LB      = 0x20,
		LW      = 0x23,
		LBU     = 0x24,
		SB      = 0x28,
		SW      = 0x2b,
		LD      = 0x37,
		SWC1    = 0x39,
		SD      = 0x3f
	};
	enum SPECIAL {
		SLL     = 0x00,
		SRL     = 0x02,
		SRA     = 0x03,
		JR      = 0x08,
		JALR    = 0x09,
		MOVN    = 0x0b,
		SYSCALL = 0x0c,
		SYNC    = 0x0f,
		MFLO    = 0x12,
		MULT    = 0x18,
		DIVU    = 0x1b,
		ADDU    = 0x21,
		SUBU    = 0x23,
		AND     = 0x24,
		OR      = 0x25,
		SLT     = 0x2a,
		SLTU    = 0x2b,
		DADDU   = 0x2d,
		DSLL    = 0x38,
		DSRL    = 0x3a
	};
	enum REGIMM {
		BGEZ = 0x01
	};
	enum COP0 {
		MFC0 = 0x00,
		MTC0 = 0x04,
		TLB  = 0x10
	};
	enum TLB {
		TLBWI = 0x02
	};

	// HLE SYSCALLs
	void Syscall(u64 v1);
	void SetGsCrt(bool interlaced, int display_mode, bool frame);
	void GsPutIMR(u64 value);
};