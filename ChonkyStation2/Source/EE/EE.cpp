#pragma warning(disable : 4996) // Stupid fread warning
#include "EE.h"

//#define FASTBOOT

#ifdef LOG_EE
#define trace(fmt, ...) \
if (traceb) { \
	Helpers::Debug((fmt), __VA_ARGS__); \
}
#else
#define trace
#endif

EE::EE(Memory* memptr) {
	mem = memptr;
	cop0r[15].b64[0] = 0x69;
	mem->pc = &pc;

	// Setup timers
	mem->t0.intc_stat = &mem->intc_stat;
	mem->t1.intc_stat = &mem->intc_stat;
	mem->t2.intc_stat = &mem->intc_stat;
	mem->t3.intc_stat = &mem->intc_stat;
	mem->t0.id = 0;
	mem->t1.id = 1;
	mem->t2.id = 2;
	mem->t3.id = 3;

	vu.vu0_code_mem = mem->vu0_code_mem;
	vu.vu0_data_mem = mem->vu0_data_mem;
}

void EE::Exception(unsigned int exception, bool int1) {
	cop0r[13].b64[0] &= ~0x7c;	// CAUSE
	cop0r[13].b64[0] |= (u64)(exception & 0x1f) << 2;  // CAUSE
	cop0r[13].b64[0] &= ~(1 << 31);	// CAUSE

	if (branch_pc != std::nullopt) {
		cop0r[14].b64[0] = pc - 4; // EPC
		cop0r[13].b64[0] |= (1u << 31);	// CAUSE
		branch_pc = std::nullopt;
	}
	else {
		cop0r[14].b64[0] = pc;	// EPC
	}
	cop0r[12].b64[0] |= 2;	// STATUS
	if (exception == Exceptions::Interrupt) pc = 0x80000200;
	else pc = 0x80000180;

	cop0r[13].b64[0] &= ~(1 << 11);
	cop0r[13].b64[0] |= int1 << 11; // CAUSE
}

void EE::Execute(Instruction instr) {
	gprs[0].b64[0] = 0;
	gprs[0].b64[1] = 0;

	// Handle branch delays
	if (branch_pc.has_value()) {
		pc = branch_pc.value();
		pc -= 4;
		branch_pc = std::nullopt;
	}

	switch ((instr.raw >> 26) & 0x3f) {
	case SPECIAL: {
		switch (instr.raw & 0x3f) {
		case SLL: {
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] << instr.sa);
			trace(Helpers::Log::EEd, "sll %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SRL: {
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] >> instr.sa);
			trace(Helpers::Log::EEd, "srl %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SRA: {
			gprs[instr.rd].b64[0] = (u64)(s32)((s32)gprs[instr.rt].b32[0] >> instr.sa);
			trace(Helpers::Log::EEd, "sra %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SLLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x1f;
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] << shift);
			trace(Helpers::Log::EEd, "sllv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SRLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x1f;
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] >> shift);
			trace(Helpers::Log::EEd, "srlv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SRAV: {
			const int shift = gprs[instr.rs].b64[0] & 0x1f;
			gprs[instr.rd].b64[0] = (u64)(s32)((s32)gprs[instr.rt].b32[0] >> shift);
			trace(Helpers::Log::EEd, "srav %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case JR: {
			branch_pc = gprs[instr.rs].b32[0];
			trace(Helpers::Log::EEd, "jr %s\n", gpr[instr.rs.Value()].c_str());
			break;
		}
		case JALR: {
			branch_pc = gprs[instr.rs].b32[0];
			gprs[instr.rd].b64[0] = (pc + 8);
			trace(Helpers::Log::EEd, "jalr %s %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str());
			break;
		}
		case MOVZ: {
			if (gprs[instr.rt].b64[0] == 0) gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "movz %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MOVN: {
			if (gprs[instr.rt].b64[0] != 0) gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "movn %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SYSCALL: {
#ifndef BIOS_HLE
			//printf("Syscall %xh @ 0x%08x\n", gprs[3].b64[0], pc);
			Exception(Exceptions::ExSYSCALL);
			return;
#else
			Syscall(gprs[3].b64[0]);
			break;
#endif
		}
		case SYNC: {
			trace(Helpers::Log::EEd, "sync\n");
			break;
		}
		case MFHI: {
			gprs[instr.rd].b64[0] = hi.b64[0];
			trace(Helpers::Log::EEd, "mfhi %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MTHI: {
			hi.b64[0] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "mthi %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MFLO: {
			gprs[instr.rd].b64[0] = lo.b64[0];
			trace(Helpers::Log::EEd, "mflo %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MTLO: {
			lo.b64[0] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "mtlo %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case DSLLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = gprs[instr.rt].b64[0] << shift;
			trace(Helpers::Log::EEd, "dsllv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSRLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = gprs[instr.rt].b64[0] >> shift;
			trace(Helpers::Log::EEd, "dsrlv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSRAV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = (s64)gprs[instr.rt].b64[0] >> shift;
			trace(Helpers::Log::EEd, "dsrav %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MULT: {
			s64 result = (s32)gprs[instr.rs].b32[0] * (s32)gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)(result & 0xffffffff);
			lo.b64[0] = (u64)(s32)(result & 0xffffffff);
			hi.b64[0] = (u64)(s32)(((u64)result >> 32) & 0xffffffff);
			trace(Helpers::Log::EEd, "mult %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case MULTU: {
			u64 result = gprs[instr.rs].b32[0] * gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)(result & 0xffffffff);
			lo.b64[0] = (u64)(s32)(result & 0xffffffff);
			hi.b64[0] = (u64)(s32)((result >> 32) & 0xffffffff);
			trace(Helpers::Log::EEd, "multu %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case DIV: {
			if (gprs[instr.rt].b32[0] == 0) {
				Helpers::Panic("Division by 0 (DIV)");
			}
			s32 quotient = (s32)gprs[instr.rs].b32[0] / (s32)gprs[instr.rt].b32[0];
			s32 remainder = (s32)gprs[instr.rs].b32[0] % (s32)gprs[instr.rt].b32[0];
			lo.b64[0] = (u64)quotient;
			hi.b64[0] = (u64)remainder;
			trace(Helpers::Log::EEd, "div %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case DIVU: {
			if (gprs[instr.rt].b32[0] == 0) {
				Helpers::Panic("Division by 0 (DIVU)");
			}
			u32 quotient = gprs[instr.rs].b32[0] / gprs[instr.rt].b32[0];
			u32 remainder = gprs[instr.rs].b32[0] % gprs[instr.rt].b32[0];
			lo.b64[0] = (u64)(s32)quotient;
			hi.b64[0] = (u64)(s32)remainder;
			trace(Helpers::Log::EEd, "divu %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case ADD: {	// TODO: Overflow
			u32 result = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			trace(Helpers::Log::EEd, "add %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case ADDU: {
			u32 result = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			trace(Helpers::Log::EEd, "addu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SUB: {	// TODO: Overflow
			u64 result = gprs[instr.rs].b32[0] - gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			trace(Helpers::Log::EEd, "sub %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SUBU: {
			u64 result = gprs[instr.rs].b32[0] - gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			trace(Helpers::Log::EEd, "subu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case AND: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] & gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "and %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case OR: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "or %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case XOR: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] ^ gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "xor %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case NOR: {
			gprs[instr.rd].b64[0] = ~(gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "nor %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MTSA: {
			// TODO
			printf("WARNING: UNIMPLEMENTED MTSA\n");
			break;
		}
		case SLT: {
			gprs[instr.rd].b64[0] = ((s64)gprs[instr.rs].b64[0] < (s64)gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "slt %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SLTU: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] < gprs[instr.rt].b64[0]);
			trace(Helpers::Log::EEd, "sltu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DADDU: {
			gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0] + gprs[instr.rt].b64[0];
			trace(Helpers::Log::EEd, "daddu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSUB: { // TODO: Overflow
			gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0] - gprs[instr.rt].b64[0];
			trace(Helpers::Log::EEd, "dsub %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSUBU: {
			gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0] - gprs[instr.rt].b64[0];
			trace(Helpers::Log::EEd, "dsubu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MFSA: {
			// TODO
			printf("WARNING: UNIMPLEMENTED MFSA\n");
			break;
		}
		case TGE: {
			if (gprs[instr.rs].b64[0] >= gprs[instr.rt].b64[0]) Exception(Exceptions::Trap);
			break;
		}
		case TEQ: {
			if (gprs[instr.rs].b64[0] == gprs[instr.rt].b64[0]) Exception(Exceptions::Trap);
			break;
		}
		case DSLL: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] << instr.sa);
			trace(Helpers::Log::EEd, "dsll %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRL: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] >> instr.sa);
			trace(Helpers::Log::EEd, "dsrl %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRA: {
			gprs[instr.rd].b64[0] = ((s64)gprs[instr.rt].b64[0] >> instr.sa);
			trace(Helpers::Log::EEd, "dsra %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSLL32: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] << (instr.sa + 32));
			trace(Helpers::Log::EEd, "dsll32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRL32: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] >> (instr.sa + 32));
			trace(Helpers::Log::EEd, "dsrl32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRA32: {
			gprs[instr.rd].b64[0] = ((s64)gprs[instr.rt].b64[0] >> (instr.sa + 32));
			trace(Helpers::Log::EEd, "dsra32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		default: 
			Helpers::Panic("Unimplemented SPECIAL instruction 0x%02x @ 0x%08x\n", instr.raw & 0x3f, pc);
		}
		break;
	}
	case REGIMM: {
		switch (instr.rt) {
		case BLTZ: {
			u32 offset = instr.imm;
			if ((s64)gprs[instr.rs].b64[0] < 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			trace(Helpers::Log::EEd, "bltz %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case BGEZ: {
			u32 offset = instr.imm;
			if ((s64)gprs[instr.rs].b64[0] >= 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			trace(Helpers::Log::EEd, "bgez %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case BLTZL: {
			u32 offset = instr.imm;
			if ((s64)gprs[instr.rs].b64[0] < 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			else {
				pc += 4;
			}
			trace(Helpers::Log::EEd, "bltzl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case BGEZL: {
			u32 offset = instr.imm;
			if ((s64)gprs[instr.rs].b64[0] >= 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			else {
				pc += 4;
			}
			trace(Helpers::Log::EEd, "bgezl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case BGEZAL: {
			u32 offset = instr.imm;
			gprs[31].b64[0] = (pc + 8);
			if ((s64)gprs[instr.rs].b64[0] >= 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			trace(Helpers::Log::EEd, "bgezal %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case MTSAH: {
			// TODO
			printf("WARNING: UNIMPLEMENTED MTSAH\n");
			break;
		}
		default:
			Helpers::Panic("Unimplemented REGIMM instruction 0x%02x @ 0x%08x\n", instr.rt.Value(), pc);
		}
		break;
	}
	case J: {
		u32 offset = instr.jump_imm;
		branch_pc = (((pc + 4) & 0xf0000000) | (offset << 2));
		trace(Helpers::Log::EEd, "j 0x%x\n", offset);
		break;
	}
	case JAL: {
		u32 offset = instr.jump_imm;
		branch_pc = (((pc + 4) & 0xf0000000) | (offset << 2));
		gprs[31].b64[0] = (pc + 8);
		trace(Helpers::Log::EEd, "jal 0x%x\n", offset);
		break;
	}
	case BEQ: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] == gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		trace(Helpers::Log::EEd, "beq %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BNE: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] != gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		trace(Helpers::Log::EEd, "bne %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BLEZ: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] <= 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		trace(Helpers::Log::EEd, "blez %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case BGTZ: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] > 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		trace(Helpers::Log::EEd, "bgtz %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case ADDI: {	// TODO: Overflow
		gprs[instr.rt].b64[0] = (s32)(gprs[instr.rs].b32[0] + (s16)instr.imm);
		trace(Helpers::Log::EEd, "addi %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ADDIU: {
		trace(Helpers::Log::EEd, "addiu %s, %s, 0x%04x ; 0x%x + 0x%x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value(), gprs[instr.rs].b32[0], (s16)instr.imm);
		gprs[instr.rt].b64[0] = (s32)(gprs[instr.rs].b32[0] + (s16)instr.imm);
		break;
	}
	case SLTI: {
		gprs[instr.rt].b64[0] = ((s64)gprs[instr.rs].b64[0] < (s64)(s16)instr.imm);
		trace(Helpers::Log::EEd, "slti %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case SLTIU: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] < (u64)(s16)instr.imm);
		trace(Helpers::Log::EEd, "sltiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ANDI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] & instr.imm);
		trace(Helpers::Log::EEd, "andi %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ORI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] | instr.imm);
		trace(Helpers::Log::EEd, "ori %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case XORI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] ^ instr.imm);
		trace(Helpers::Log::EEd, "xori %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case LUI: {
		gprs[instr.rt].b64[0] = (u64)(s32)(instr.imm << 16);
		trace(Helpers::Log::EEd, "lui %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value());
		break;
	}
	case COP0: {
		switch ((instr.raw >> 21) & 0x1f) {
		case MFC0: {
			gprs[instr.rt].b32[0] = cop0r[instr.rd].b32[0];
			trace(Helpers::Log::EEd, "mfc0 %s, cop0r%d\n", gpr[instr.rt.Value()].c_str(), instr.rd.Value());
			break;
		}
		case MTC0: {
			cop0r[instr.rd].b32[0] = gprs[instr.rt].b32[0];
			trace(Helpers::Log::EEd, "mtc0 %s, cop0r%d\n", gpr[instr.rt.Value()].c_str(), instr.rd.Value());
			break;
		}
		case BC0: {
			printf("Unimplemented BC0!\n");
			break;
		}
		case TLB: {
			switch (instr.raw & 0x3f) {
			case TLBWI: {
				printf("tlbwi!\n");
				break;
			}
			case ERET: {
				if (cop0r[12].b64[0] & (1 << 2)) {
					pc = cop0r[30].b64[0];
					cop0r[12].b64[0] &= ~(1 << 2);
				}
				else {
					pc = cop0r[14].b64[0];
					cop0r[12].b64[0] &= ~2;
				}
				trace(Helpers::Log::EEd, "eret\n");
#ifdef FASTBOOT
				// Fastboot hack
				if (pc == 0x82000) {
					const char* dir = "cdrom0:\\\\SLUS_211.13;1"; // Atelier Iris
					//const char* dir = "cdrom0:\\\\SCUS_973.28;1"; // Gran Turismo 4
					//const char* dir = "cdrom0:\\\\SLUS_212.67;1"; // Need for Speed Most Wanted
					//const char* dir = "cdrom0:\\\\SLUS_217.28;1"; // Crash - Mind Over Mutant
					//const char* dir = "cdrom0:\\\\SLPM_664.72;1"; // Planetarian
					//const char* dir = "cdrom0:\\\\SLES_525.63;1"; // FIFA 05
					std::memcpy(&mem->ram[0x89580], dir, strlen(dir));
				}
				mem->fastbooted = true;
#endif
				if (sideload_elf) {
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2autotests-master\\ps2autotests-master\\tests\\cpu\\ee\\alu.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_01\\demo1.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02a\\demo2a.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02b\\demo2b.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\graph.elf");
					pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\cube.elf");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\SLES_525.63");
					//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\SLUS_211.13");
					//branch_pc = std::nullopt;
					//traceb = true;
					sideload_elf = false;
				}
				return;
			}
			case EI: {
				if (((cop0r[12].b64[0] >> 17) & 1) || ((cop0r[12].b64[0] >> 1) & 1) || ((cop0r[12].b64[0] >> 2) & 1) || (((cop0r[12].b64[0] >> 3) & 3) == 0))
					cop0r[12].b64[0] |= 1 << 16;
				trace(Helpers::Log::EEd, "ei\n");
				break;
			}
			case DI: {
				if (((cop0r[12].b64[0] >> 17) & 1) || ((cop0r[12].b64[0] >> 1) & 1) || ((cop0r[12].b64[0] >> 2) & 1) || (((cop0r[12].b64[0] >> 3) & 3) == 0))
					cop0r[12].b64[0] &= ~(1 << 16);
				trace(Helpers::Log::EEd, "di\n");
				break;
			}
			default:
				Helpers::Panic("Unimplemented TLB/Exception instruction 0x%02x @ 0x%08x\n", instr.raw & 0x3f, pc);
			}
			break;
		}
		default:
			Helpers::Panic("Unimplemented COP0 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 21) & 0x1f, pc);
		}
		break;
	}
	case COP1: {
		switch (instr.rs) {
		case FPU::MFC1: {
			gprs[instr.rt].b64[0] = (u64)(s32)fprs[instr.fs];
			trace(Helpers::Log::EEd, "mfc1 r%d, f%d\n", instr.rt, instr.fs);
			break;
		}
		case FPU::CFC1: {
			if (instr.fs != 31) {
				Helpers::Panic("CFC1 with instr.rs != 31\n");
			}
			gprs[instr.rt].b32[0] = fcr31;
			trace(Helpers::Log::EEd, "cfc1\n");
			break;
		}
		case FPU::MTC1: {
			fprs[instr.fd] = *(u32*)&gprs[instr.rt].b32[0];
			Helpers::Debug(Helpers::Log::NOCOND, "mtc1 r%d, f%d ; (f%d = %f)\n", instr.rt.Value(), instr.rd.Value(), instr.rd.Value(), *(float*)&fprs[instr.fd]);
			break;
		}
		case FPU::CTC1: {
			if (instr.fs != 31 && (instr.fs != 0)) {
				Helpers::Panic("CTC1 with instr.rs != 31 or 0 (instr.rs = %d)\n", instr.rs.Value());
			}
			trace(Helpers::Log::EEd, "ctc1\n");
			if (instr.fs == 0) break;
			fcr31 = gprs[instr.rt].b32[0];
			break;
		}
		case FPU::BC1: {
			switch (instr.ft) {
			case BC1::BC1F: {
				u32 offset = instr.imm;
				if (!((fcr31 >> 23) & 1)) {
					branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
				}
				trace(Helpers::Log::EEd, "bc1f\n");
				break;
			}
			case BC1::BC1T: {
				u32 offset = instr.imm;
				if ((fcr31 >> 23) & 1) {
					branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
				}
				trace(Helpers::Log::EEd, "bc1t\n");
				break;
			}
			case BC1::BC1FL: {
				u32 offset = instr.imm;
				if (!((fcr31 >> 23) & 1)) {
					branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
				}
				else {
					pc += 4;
				}
				trace(Helpers::Log::EEd, "bc1fl\n");
				break;
			}
			case BC1::BC1TL: {
				u32 offset = instr.imm;
				if ((fcr31 >> 23) & 1) {
					branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
				}
				else {
					pc += 4;
				}
				trace(Helpers::Log::EEd, "bc1tl\n");
				break;
			}
			default:
				Helpers::Panic("Unimplemented FPU BC1 instruction 0x%02x\n", instr.ft.Value());
			}
			break;
		}
		// FPU.S
		case 0x10: {
			switch (instr.raw & 0x3f) {
			case FPU_S::ADD_S: {
				float res = *(float*)&fprs[instr.fs] + *(float*)&fprs[instr.ft];
				fprs[instr.fd] = *(u32*)&res;
				Helpers::Debug(Helpers::Log::NOCOND, "add.s f%d, f%d, f%d ; (f%d = %f)\n", instr.fd.Value(), instr.fs.Value(), instr.ft.Value(), instr.fd.Value(), *(float*)&fprs[instr.fd]);
				break;
			}
			case FPU_S::SUB_S: {
				float res = *(float*)&fprs[instr.fs] - *(float*)&fprs[instr.ft];
				fprs[instr.fd] = *(u32*)&res;
				Helpers::Debug(Helpers::Log::NOCOND, "add.s f%d, f%d, f%d ; (f%d = %f)\n", instr.fd.Value(), instr.fs.Value(), instr.ft.Value(), instr.fd.Value(), *(float*)&fprs[instr.fd]);
				break;
			}
			case FPU_S::MUL_S: {
				float res = *(float*)&fprs[instr.fs] * *(float*)&fprs[instr.ft];
				fprs[instr.fd] = *(u32*)&res;
				Helpers::Debug(Helpers::Log::NOCOND, "mul.s f%d, f%d, f%d ; (f%d = %f)\n", instr.fd.Value(), instr.fs.Value(), instr.ft.Value(), instr.fd.Value(), *(float*)&fprs[instr.fd]);
				break;
			}
			case FPU_S::DIV_S: {
				Helpers::Debug(Helpers::Log::NOCOND, "div.s f%d, f%d, f%d ", instr.fd.Value(), instr.fs.Value(), instr.ft.Value());
				if (*(float*)&fprs[instr.ft] == 0.f) {
					Helpers::Panic("\nDIV.S DIVISION BY 0 (%f / %f)\n", *(float*)&fprs[instr.fs], *(float*)&fprs[instr.ft]);
				}
				float res = *(float*)&fprs[instr.fs] / *(float*)&fprs[instr.ft];
				fprs[instr.fd] = *(u32*)&res;
				Helpers::Debug(Helpers::Log::NOCOND, "; (f%d = %f)\n", instr.fd.Value(), *(float*)&fprs[instr.fd]);
				break;
			}
			case FPU_S::SQRT_S: {
				float res = sqrt(*(float*)&fprs[instr.ft]);
				fprs[instr.fd] = *(u32*)&res;
				trace(Helpers::Log::EEd, "sqrt.s f%d, f%d\n", instr.fd, instr.ft);
				break;
			}
			case FPU_S::MOV_S: {
				fprs[instr.fd] = *(u32*)&fprs[instr.fs];
				trace(Helpers::Log::EEd, "mov.s f%d, f%d\n", instr.fd, instr.fs);
				break;
			}
			case FPU_S::NEG_S: {
				fprs[instr.fd] = -*(s32*)&fprs[instr.fs];
				trace(Helpers::Log::EEd, "neg.s f%d, f%d\n", instr.fd, instr.fs);
				break;
			}
			case FPU_S::ADDA_S: {
				acc = *(float*)&fprs[instr.fs] + *(float*)&fprs[instr.ft];
				trace(Helpers::Log::EEd, "adda.s f%d, f%d\n", instr.fs, instr.ft);
				break;
			}
			case FPU_S::MADD_S: {
				float res = *(float*)&fprs[instr.fs] * *(float*)&fprs[instr.ft] + acc;
				fprs[instr.fd] = *(u32*)&res;
				Helpers::Debug(Helpers::Log::NOCOND, "madd.s f%d, f%d, f%d ; (f%d = %f)\n", instr.fd.Value(), instr.fs.Value(), instr.ft.Value(), instr.fd.Value(), *(float*)&fprs[instr.fd]);
				break;
			}
			case FPU_S::CVT_W: {
				fprs[instr.fd] = (s32)*(float*)&fprs[instr.fs];
				trace(Helpers::Log::EEd, "cvt.w f%d, f%d\n", instr.fd, instr.fs);
				break;
			}
			case FPU_S::CEQ_S: {
				fcr31 &= ~(1 << 23);
				fcr31 |= ((*(float*)&fprs[instr.fs] == *(float*)&fprs[instr.ft]) << 23);
				trace(Helpers::Log::EEd, "c.eq.s f%d, f%d\n", instr.fs, instr.ft);
				break;
			}
			case FPU_S::CLT_S: {
				fcr31 &= ~(1 << 23);
				fcr31 |= ((*(float*)&fprs[instr.fs] < *(float*)&fprs[instr.ft]) << 23);
				trace(Helpers::Log::EEd, "c.lt.s f%d, f%d\n", instr.fs, instr.ft);
				break;
			}
			case FPU_S::CLE_S: {
				fcr31 &= ~(1 << 23);
				fcr31 |= ((*(float*)&fprs[instr.fs] <= *(float*)&fprs[instr.ft]) << 23);
				trace(Helpers::Log::EEd, "c.le.s f%d, f%d\n", instr.fs, instr.ft);
				break;
			}
			default:
				Helpers::Panic("Unimplemented FPU S instruction 0x%02x\n", instr.raw & 0x3f);
			}
			break;
		}
		// FPU.W
		case 0x14: {
			switch (instr.raw & 0x3f) {
			// CVT.S
			case 0x20: {
				fprs[instr.fd] = (float)(s32)fprs[instr.fs];
				trace(Helpers::Log::EEd, "cvt.s f%d, f%d\n", instr.fd, instr.fs);
				break;
			}
			default:
				Helpers::Panic("Unimplemented FPU W instruction 0x%02x\n", instr.raw & 0x3f);
			}
			break;
		}
		default:
			Helpers::Panic("Unimplemented FPU instruction 0x%02x\n", instr.rs.Value());
		}
		break;
	}
	case COP2: {
		switch (instr.rs) {
		case 0x01: {	// QMFC2
			gprs[instr.rt].b32[0] = vu.vf[instr.fd][0];
			gprs[instr.rt].b32[1] = vu.vf[instr.fd][1];
			gprs[instr.rt].b32[2] = vu.vf[instr.fd][2];
			gprs[instr.rt].b32[3] = vu.vf[instr.fd][3];
			trace(Helpers::Log::EEd, "qmfc2\n");
			break;
		}
		case 0x02: {	// CFC2	(stubbed)
			gprs[instr.rt].b32[0] = 0;
			trace(Helpers::Log::EEd, "cfc2\n");
			break;
		}
		case 0x05: {	// QMTC2
			vu.vf[instr.fd][0] = gprs[instr.rt].b32[0];
			vu.vf[instr.fd][1] = gprs[instr.rt].b32[1];
			vu.vf[instr.fd][2] = gprs[instr.rt].b32[2];
			vu.vf[instr.fd][3] = gprs[instr.rt].b32[3];
			trace(Helpers::Log::EEd, "qmtc2\n");
			break;
		}
		case 0x06: {	// CTC2 (stubbed)
			if (instr.is < 16) {
				vu.vi[instr.is] = gprs[instr.rt].b16[0];
			}
			trace(Helpers::Log::EEd, "ctc2\n");
			break;
		}
		default:
			vu.ExecVU0Macro(instr);
		}
		break;
	}
	case BEQL: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] == gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		else {
			pc += 4;
		}
		trace(Helpers::Log::EEd, "beql %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BNEL: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] != gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		else {
			pc += 4;
		}
		trace(Helpers::Log::EEd, "bnel %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BLEZL: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] <= 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		else {
			pc += 4;
		}
		trace(Helpers::Log::EEd, "blezl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case BGTZL: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] > 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		else {
			pc += 4;
		}
		trace(Helpers::Log::EEd, "bgtzl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case DADDIU: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] + (u64)(s16)instr.imm);
		trace(Helpers::Log::EEd, "daddiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case LDL: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = ((address & 7) ^ 7) * 8;
		u64 data_temp = mem->Read<u64>(address & ~7);
		u64 rt_temp = gprs[instr.rt].b64[0] & ~(0xffffffffffffffff >> shift);
		data_temp >>= shift;
		gprs[instr.rt].b64[0] = data_temp | rt_temp;
		trace(Helpers::Log::EEd, "ldl %s, 0x%04x(%s) ; %s merge mem[0x%08x] (0x%016x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data_temp | rt_temp);
		break;
	}
	case LDR: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = (address & 7) * 8;
		u64 data_temp = mem->Read<u64>(address & ~7);
		u64 rt_temp = gprs[instr.rt].b64[0] & ~(0xffffffffffffffff << shift);
		data_temp <<= shift;
		gprs[instr.rt].b64[0] = data_temp | rt_temp;
		trace(Helpers::Log::EEd, "ldr %s, 0x%04x(%s) ; %s merge mem[0x%08x] (0x%016x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data_temp | rt_temp);
		break;
	}
	case MMI: {
		switch (instr.raw & 0x3f) {
		case MADD: {
			s64 result = (s32)gprs[instr.rs].b32[0] * (s32)gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)(result & 0xffffffff);
			lo.b64[0] += (u64)(s32)(result & 0xffffffff);
			hi.b64[0] += (u64)(s32)(((u64)result >> 32) & 0xffffffff);
			trace(Helpers::Log::EEd, "madd %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case PLZCW: {
			u32 a = gprs[instr.rs].b32[0];
			u32 b = gprs[instr.rs].b32[1];
			if (a & (1 << 31)) a = ~a;
			if (b & (1 << 31)) b = ~b;
			gprs[instr.rd].b32[0] = __lzcnt(a) - 1;
			gprs[instr.rd].b32[1] = __lzcnt(b) - 1;
			trace(Helpers::Log::EEd, "plzcw %s %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str());
			break;
		}
		case MMI0: {
			switch ((instr.raw >> 6) & 0x1f) {
			case PSUBW: {
				for (int i = 0; i < 4; i++) {
					gprs[instr.rd].b32[i] = gprs[instr.rs].b32[i] - gprs[instr.rt].b32[i];
				}
				trace(Helpers::Log::EEd, "psubw %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PSUBB: {
				for (int i = 0; i < 16; i++) {
					gprs[instr.rd].b8[i] = gprs[instr.rs].b8[i] - gprs[instr.rt].b8[i];
				}
				trace(Helpers::Log::EEd, "psubb %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PEXTLW: {
				gprs[instr.rd].b32[0] = gprs[instr.rt].b32[0];
				gprs[instr.rd].b32[1] = gprs[instr.rs].b32[0];
				gprs[instr.rd].b32[2] = gprs[instr.rt].b32[1];
				gprs[instr.rd].b32[3] = gprs[instr.rs].b32[1];
				trace(Helpers::Log::EEd, "pextlw %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PEXTLH: {
				gprs[instr.rd].b16[0] = gprs[instr.rt].b16[0];
				gprs[instr.rd].b16[1] = gprs[instr.rs].b16[0];
				gprs[instr.rd].b16[2] = gprs[instr.rt].b16[1];
				gprs[instr.rd].b16[3] = gprs[instr.rs].b16[1];
				gprs[instr.rd].b16[4] = gprs[instr.rt].b16[2];
				gprs[instr.rd].b16[5] = gprs[instr.rs].b16[2];
				gprs[instr.rd].b16[6] = gprs[instr.rt].b16[3];
				gprs[instr.rd].b16[7] = gprs[instr.rs].b16[3];
				trace(Helpers::Log::EEd, "pextlh %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PADDSH: {
				// TODO: Saturate overflow to 0x7fff, saturate underflow to 0x8000
				for (int i = 0; i < 8; i++) {
					gprs[instr.rd].b16[i] = gprs[instr.rs].b16[i] + gprs[instr.rt].b16[i];
				}
				trace(Helpers::Log::EEd, "paddsh %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PEXT5: {
				gprs[instr.rd].b8[0] = (gprs[instr.rt].b16[0] & 0x1f) << 2;
				gprs[instr.rd].b8[1] = ((gprs[instr.rt].b16[0] >> 5) & 0x1f) << 2;
				gprs[instr.rd].b8[2] = ((gprs[instr.rt].b16[0] >> 10) & 0x1f) << 2;
				gprs[instr.rd].b8[3] = (gprs[instr.rt].b16[0] >> 15) << 7;

				gprs[instr.rd].b8[4] = (gprs[instr.rt].b16[2] & 0x1f) << 2;
				gprs[instr.rd].b8[5] = ((gprs[instr.rt].b16[2] >> 5) & 0x1f) << 2;
				gprs[instr.rd].b8[6] = ((gprs[instr.rt].b16[2] >> 10) & 0x1f) << 2;
				gprs[instr.rd].b8[7] = (gprs[instr.rt].b16[2] >> 15) << 7;

				gprs[instr.rd].b8[8] = (gprs[instr.rt].b16[4] & 0x1f) << 2;
				gprs[instr.rd].b8[9] = ((gprs[instr.rt].b16[4] >> 5) & 0x1f) << 2;
				gprs[instr.rd].b8[10] = ((gprs[instr.rt].b16[4] >> 10) & 0x1f) << 2;
				gprs[instr.rd].b8[11] = (gprs[instr.rt].b16[4] >> 15) << 7;

				gprs[instr.rd].b8[12] = (gprs[instr.rt].b16[6] & 0x1f) << 2;
				gprs[instr.rd].b8[13] = ((gprs[instr.rt].b16[6] >> 5) & 0x1f) << 2;
				gprs[instr.rd].b8[14] = ((gprs[instr.rt].b16[6] >> 10) & 0x1f) << 2;
				gprs[instr.rd].b8[15] = (gprs[instr.rt].b16[6] >> 15) << 7;
				trace(Helpers::Log::EEd, "pext5 %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PPAC5: {
				gprs[instr.rd].b16[0] = ((gprs[instr.rt].b8[0] >> 3) << 0) | ((gprs[instr.rt].b8[1] >> 3) << 5) | ((gprs[instr.rt].b8[2] >> 3) << 10) | ((gprs[instr.rt].b8[3] >> 7) << 15);
				gprs[instr.rd].b16[1] = 0;
				gprs[instr.rd].b16[2] = ((gprs[instr.rt].b8[4] >> 3) << 0) | ((gprs[instr.rt].b8[5] >> 3) << 5) | ((gprs[instr.rt].b8[6] >> 3) << 10) | ((gprs[instr.rt].b8[7] >> 7) << 15);
				gprs[instr.rd].b16[3] = 0;
				gprs[instr.rd].b16[4] = ((gprs[instr.rt].b8[8] >> 3) << 0) | ((gprs[instr.rt].b8[9] >> 3) << 5) | ((gprs[instr.rt].b8[10] >> 3) << 10) | ((gprs[instr.rt].b8[11] >> 7) << 15);
				gprs[instr.rd].b16[5] = 0;
				gprs[instr.rd].b16[6] = ((gprs[instr.rt].b8[12] >> 3) << 0) | ((gprs[instr.rt].b8[13] >> 3) << 5) | ((gprs[instr.rt].b8[14] >> 3) << 10) | ((gprs[instr.rt].b8[15] >> 7) << 15);
				gprs[instr.rd].b16[7] = 0;
				trace(Helpers::Log::EEd, "ppac5 %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			default:
				Helpers::Panic("Unimplemented MMI0 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 6) & 0x1f, pc);
			}
			break;
		}
		case MMI2: {
			switch ((instr.raw >> 6) & 0x1f) {
			case PMFHI: {
				gprs[instr.rd].b64[0] = hi.b64[0];
				gprs[instr.rd].b64[1] = hi.b64[1];
				trace(Helpers::Log::EEd, "pmfhi %s\n", gpr[instr.rd.Value()].c_str());
				break;
			}
			case PMFLO: {
				gprs[instr.rd].b64[0] = lo.b64[0];
				gprs[instr.rd].b64[1] = lo.b64[1];
				trace(Helpers::Log::EEd, "pmflo %s\n", gpr[instr.rd.Value()].c_str());
				break;
			}
			case PCPYLD: {
				gprs[instr.rd].b64[1] = gprs[instr.rs].b64[0];
				gprs[instr.rd].b64[0] = gprs[instr.rt].b64[0];
				trace(Helpers::Log::EEd, "pcpyld %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PAND: {
				gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] & gprs[instr.rt].b64[0]);
				gprs[instr.rd].b64[1] = (gprs[instr.rs].b64[1] & gprs[instr.rt].b64[1]);
				trace(Helpers::Log::EEd, "pand %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
				break;
			}
			case PXOR: {
				gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] ^ gprs[instr.rt].b64[0]);
				gprs[instr.rd].b64[1] = (gprs[instr.rs].b64[1] ^ gprs[instr.rt].b64[1]);
				trace(Helpers::Log::EEd, "pxor %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
				break;
			}
			default:
				Helpers::Panic("Unimplemented MMI2 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 6) & 0x1f, pc);
			}
			break;
		}
		case MFHI1: {
			gprs[instr.rd].b64[0] = hi.b64[1];
			trace(Helpers::Log::EEd, "mfhi1 %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MTHI1: {
			hi.b64[1] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "mthi1 %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MFLO1: {
			gprs[instr.rd].b64[0] = lo.b64[1];
			trace(Helpers::Log::EEd, "mflo1 %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MTLO1: {
			lo.b64[1] = gprs[instr.rs].b64[0];
			trace(Helpers::Log::EEd, "mtlo1 %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MULT1: {
			s64 result = (s32)gprs[instr.rs].b32[0] * (s32)gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)(result & 0xffffffff);
			lo.b64[1] = (u64)(s32)(result & 0xffffffff);
			hi.b64[1] = (u64)(s32)(((u64)result >> 32) & 0xffffffff);
			trace(Helpers::Log::EEd, "mult1 %s %s %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case DIV1: {
			if (gprs[instr.rt].b32[0] == 0) {
				Helpers::Panic("Division by 0 (DIV1)");
			}
			s32 quotient = (s32)gprs[instr.rs].b32[0] / (s32)gprs[instr.rt].b32[0];
			s32 remainder = (s32)gprs[instr.rs].b32[0] % (s32)gprs[instr.rt].b32[0];
			lo.b64[1] = (u64)quotient;
			hi.b64[1] = (u64)remainder;
			trace(Helpers::Log::EEd, "div1 %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case DIVU1: {
			if (gprs[instr.rt].b32[0] == 0) {
				Helpers::Panic("Division by 0 (DIVU1)");
			}
			u32 quotient = gprs[instr.rs].b32[0] / gprs[instr.rt].b32[0];
			u32 remainder = gprs[instr.rs].b32[0] % gprs[instr.rt].b32[0];
			lo.b64[1] = (u64)(s32)quotient;
			hi.b64[1] = (u64)(s32)remainder;
			trace(Helpers::Log::EEd, "divu1 %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case MMI1: {
			switch ((instr.raw >> 6) & 0x1f) {
			case PADDUW: {
				// TODO: Saturate overflow to 0xffffffff
				gprs[instr.rd].b32[0] = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
				gprs[instr.rd].b32[1] = gprs[instr.rs].b32[1] + gprs[instr.rt].b32[1];
				gprs[instr.rd].b32[2] = gprs[instr.rs].b32[2] + gprs[instr.rt].b32[2];
				gprs[instr.rd].b32[3] = gprs[instr.rs].b32[3] + gprs[instr.rt].b32[3];
				trace(Helpers::Log::EEd, "padduw %s, %s, %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PEXTUW: {
				gprs[instr.rd].b32[0] = gprs[instr.rt].b32[2];
				gprs[instr.rd].b32[1] = gprs[instr.rs].b32[2];
				gprs[instr.rd].b32[2] = gprs[instr.rt].b32[3];
				gprs[instr.rd].b32[3] = gprs[instr.rs].b32[3];
				trace(Helpers::Log::EEd, "pextuw %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case PCEQB: {
				for (int i = 0; i < 16; i++) {
					if (gprs[instr.rs].b8[i] == gprs[instr.rt].b8[i]) gprs[instr.rd].b8[i] = 0xff;
					else gprs[instr.rd].b8[i] = 0;
				}
				trace(Helpers::Log::EEd, "pceqb %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			default:
				Helpers::Panic("Unimplemented MMI1 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 6) & 0x1f, pc);
			}
			break;
		}
		case MMI3: {
			switch ((instr.raw >> 6) & 0x1f) {
			case PMTHI: {
				hi.b64[0] = gprs[instr.rs].b64[0];
				hi.b64[1] = gprs[instr.rs].b64[1];
				trace(Helpers::Log::EEd, "pmthi %s\n", gpr[instr.rd.Value()].c_str());
				break;
			}
			case PMTLO: {
				lo.b64[0] = gprs[instr.rs].b64[0];
				lo.b64[1] = gprs[instr.rs].b64[1];
				trace(Helpers::Log::EEd, "pmtlo %s\n", gpr[instr.rd.Value()].c_str());
				break;
			}
			case PCPYUD: {
				gprs[instr.rd].b64[0] = gprs[instr.rs].b64[1];
				gprs[instr.rd].b64[1] = gprs[instr.rt].b64[1];
				trace(Helpers::Log::EEd, "pcpyud %s, %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			case POR: {
				gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
				gprs[instr.rd].b64[1] = (gprs[instr.rs].b64[1] | gprs[instr.rt].b64[1]);
				trace(Helpers::Log::EEd, "por %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
				break;
			}
			case PNOR: {
				gprs[instr.rd].b64[0] = ~(gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
				gprs[instr.rd].b64[1] = ~(gprs[instr.rs].b64[1] | gprs[instr.rt].b64[1]);
				trace(Helpers::Log::EEd, "pnor %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
				break;
			}
			case PCPYH: {
				gprs[instr.rd].b16[0] = gprs[instr.rt].b16[0];
				gprs[instr.rd].b16[1] = gprs[instr.rt].b16[0];
				gprs[instr.rd].b16[2] = gprs[instr.rt].b16[0];
				gprs[instr.rd].b16[3] = gprs[instr.rt].b16[0];
				gprs[instr.rd].b16[4] = gprs[instr.rt].b16[4];
				gprs[instr.rd].b16[5] = gprs[instr.rt].b16[4];
				gprs[instr.rd].b16[6] = gprs[instr.rt].b16[4];
				gprs[instr.rd].b16[7] = gprs[instr.rt].b16[4];
				trace(Helpers::Log::EEd, "pcpyh %s, %s", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str());
				break;
			}
			default:
				Helpers::Panic("Unimplemented MMI3 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 6) & 0x1f, pc);
			}
			break;
		}

		case PMFHL: {
			switch (instr.sa) {
			case PMFHLLW: {
				gprs[instr.rd].b32[3] = hi.b32[2];
				gprs[instr.rd].b32[2] = lo.b32[2];
				gprs[instr.rd].b32[1] = hi.b32[0];
				gprs[instr.rd].b32[0] = lo.b32[0];
				trace(Helpers::Log::EEd, "pmfhl.lw %s", gpr[instr.rd.Value()].c_str());
				break;
			}
			default:
				Helpers::Panic("Unimplemented PMFHL: %d\n", instr.sa.Value());
			}
			break;
		}
		default:
			Helpers::Panic("Unimplemented MMI instruction 0x%02x @ 0x%08x\n", instr.raw & 0x3f, pc);
		}
		break;
	}
	case LQ: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u128 data;
		data.b64[0] = mem->Read<u64>(address);
		data.b64[1] = mem->Read<u64>(address + 8);
		gprs[instr.rt] = data;
		trace(Helpers::Log::EEd, "lq %s, 0x%04x(%s)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SQ: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		mem->Write<u64>(address, gprs[instr.rt].b64[0]);
		mem->Write<u64>(address + 8, gprs[instr.rt].b64[1]);
		trace(Helpers::Log::EEd, "sq, %s, 0x%04x(%s)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		break;
	}
	case LB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = (s8)data;
		trace(Helpers::Log::EEd, "lb %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LH: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u16 data = mem->Read<u16>(address);
		gprs[instr.rt].b64[0] = (s16)data;
		trace(Helpers::Log::EEd, "lh %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LWL: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = ((address & 3) ^ 3) * 8;
		u32 data_temp = mem->Read<u32>(address & ~3);
		u32 rt_temp = gprs[instr.rt].b64[0] & ~(0xffffffff >> shift);
		data_temp >>= shift;
		gprs[instr.rt].b32[0] = data_temp | rt_temp;
		trace(Helpers::Log::EEd, "lwl %s, 0x%04x(%s) ; %s merge mem[0x%08x] (0x%08x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data_temp | rt_temp);
		break;
	}
	case LW: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (lw addr: 0x%08x)\n", address);
		}
		u32 data = mem->Read<u32>(address);
		gprs[instr.rt].b64[0] = (s32)data;
		trace(Helpers::Log::EEd, "lw %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LBU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = data;
		trace(Helpers::Log::EEd, "lbu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LHU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u16 data = mem->Read<u16>(address);
		gprs[instr.rt].b64[0] = data;
		trace(Helpers::Log::EEd, "lhu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LWR: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = (address & 3) * 8;
		u32 data_temp = mem->Read<u32>(address & ~3);
		u32 rt_temp = gprs[instr.rt].b32[0] & ~(0xffffffff << shift);
		data_temp <<= shift;
		gprs[instr.rt].b32[0] = data_temp | rt_temp;
		trace(Helpers::Log::EEd, "lwr %s, 0x%04x(%s) ; %s merge mem[0x%08x] (0x%08x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data_temp | rt_temp);
		break;
	}
	case LWU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u32 data = mem->Read<u32>(address);
		gprs[instr.rt].b64[0] = data;
		trace(Helpers::Log::EEd, "lwu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		trace(Helpers::Log::EEd, "sb, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%02x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b8[0]);
		mem->Write<u8>(address, gprs[instr.rt].b8[0]);
		break;
	}
	case SH: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		trace(Helpers::Log::EEd, "sh, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b8[0]);
		mem->Write<u16>(address, gprs[instr.rt].b16[0]);
		break;
	}
	case SWL: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = ((address & 3) ^ 3) * 8;
		u32 data_temp = mem->Read<u32>(address & ~3);
		u32 rt_temp = gprs[instr.rt].b64[0] << shift;
		data_temp &= ~(0xffffffff << shift);
		mem->Write<u32>(address & ~3, data_temp | rt_temp);
		trace(Helpers::Log::EEd, "swl %s, 0x%04x(%s) ; mem[0x%08x] merge %s (0x%08x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gpr[instr.rt.Value()].c_str(), data_temp | rt_temp);
		break;
	}
	case SW: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		trace(Helpers::Log::EEd, "sw %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (sw)\n");
		}
		mem->Write<u32>(address, gprs[instr.rt].b32[0]);
		break;
	}
	case SDL: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = ((address & 7) ^ 7) * 8;
		u64 data_temp = mem->Read<u64>(address & ~7);
		u64 rt_temp = gprs[instr.rt].b64[0] << shift;
		data_temp &= ~(0xffffffffffffffff << shift);
		mem->Write<u64>(address & ~7, data_temp | rt_temp);
		trace(Helpers::Log::EEd, "sdl %s, 0x%04x(%s) ; mem[0x%08x] merge %s (0x%016x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gpr[instr.rt.Value()].c_str(), data_temp | rt_temp);
		break;
	}
	case SDR: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = (address & 7) * 8;
		u64 data_temp = mem->Read<u64>(address & ~7);
		u64 rt_temp = gprs[instr.rt].b64[0] >> shift;
		data_temp &= ~(0xffffffffffffffff >> shift);
		mem->Write<u64>(address & ~7, data_temp | rt_temp);
		trace(Helpers::Log::EEd, "sdr %s, 0x%04x(%s) ; mem[0x%08x] merge %s (0x%016x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gpr[instr.rt.Value()].c_str(), data_temp | rt_temp);
		break;
	}
	case SWR: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		const int shift = (address & 3) * 8;
		u32 data_temp = mem->Read<u32>(address & ~3);
		u32 rt_temp = gprs[instr.rt].b64[0] >> shift;
		data_temp &= ~(0xffffffff >> shift);
		mem->Write<u32>(address & ~3, data_temp | rt_temp);
		trace(Helpers::Log::EEd, "swr %s, 0x%04x(%s) ; mem[0x%08x] merge %s (0x%08x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gpr[instr.rt.Value()].c_str(), data_temp | rt_temp);
		break;
	}
	case CACHE: {
		trace(Helpers::Log::EEd, "cache\n");
		break;
	}
	case LD: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 7) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (ld)\n");
		}
		u64 data = mem->Read<u64>(address);
		gprs[instr.rt].b64[0] = data;
		trace(Helpers::Log::EEd, "ld %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LQC2: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		vu.vf[instr.ft][x] = mem->Read<u32>(address + 0);
		vu.vf[instr.ft][y] = mem->Read<u32>(address + 4);
		vu.vf[instr.ft][z] = mem->Read<u32>(address + 8);
		vu.vf[instr.ft][w] = mem->Read<u32>(address + 0xC);
		trace(Helpers::Log::EEd, "lqc2\n");
		break;
	}
	case LWC1: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (lw addr: 0x%08x)\n", address);
		}
		u32 data = mem->Read<u32>(address);
		fprs[instr.ft] = *(u32*)&data;
		Helpers::Debug(Helpers::Log::NOCOND, "lwc1\n");
		break;
	}
	case SWC1: {
		Helpers::Debug(Helpers::Log::NOCOND, "swc1\n");
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (swc1)\n");
		}
		mem->Write<u32>(address, fprs[instr.ft]);
		break;
	}
	case SQC2: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		mem->Write<u32>(address + 0x0, vu.vf[instr.ft][x]);
		mem->Write<u32>(address + 0x4, vu.vf[instr.ft][y]);
		mem->Write<u32>(address + 0x8, vu.vf[instr.ft][z]);
		mem->Write<u32>(address + 0xC, vu.vf[instr.ft][w]);
		trace(Helpers::Log::EEd, "sqc2\n");
		break;
	}
	case SD: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 7) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (sd) @ 0x%08x\n", pc);
		}
		mem->Write<u64>(address, gprs[instr.rt].b64[0]);
		trace(Helpers::Log::EEd, "sd, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		break;
	}
	default:
		Helpers::Panic("Unimplemented instruction 0x%02x @ 0x%08x\n", (instr.raw >> 26) & 0x3f, pc);
	}
	pc += 4;
	
}

// HLE SYSCALLs
void EE::Syscall(u64 v1) {
	/*if (v1 == 0x83) {
		printf("Ignored syscall 0x83\n");
		traceb = true;
		return;
	}*/
	for (auto& i : custom_syscalls) {
		if (i.index == v1) {
			Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) Custom Syscall 0x%x\n", i.index);
			//gprs[29].b64[0] -= 0x10;
			ra_old = gprs[31].b64[0];
			pc_old = pc;
			gprs[31].b64[0] = 0x1fc00000;
			pc = (i.address & 0x1fffffff) - 4;
			return;
		}
	}

	switch (v1) {
	case 0x02: SetGsCrt(gprs[4].b64[0], gprs[5].b64[0], gprs[6].b64[0]); break;
	case 0x12: Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) AddDmacHandler [IGNORED]\n"); break;
	case 0x16: gprs[2].b64[0] = 1; Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) _EnableDmac [IGNORED]\n"); break;
	case 0x3C: InitMainThread(gprs[4].b64[0], gprs[5].b64[0], gprs[6].b64[0], gprs[7].b64[0], gprs[8].b64[0]); break;
	case 0x3D: InitHeap(gprs[4].b64[0], gprs[5].b64[0]); break;
	case 0x40: CreateSema(gprs[4].b64[0]); break;
	case 0x41: DeleteSema(gprs[4].b64[0]); break;
	case 0x44: WaitSema(gprs[4].b64[0]); break;
	case 0x64: Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) FlushCache [IGNORED]\n"); break;
	case 0x71: GsPutIMR(gprs[4].b64[0]); break;
	case 0x74: SetSyscall(gprs[4].b64[0], gprs[5].b64[0]); break;
	case 0x77: SifSetDma(gprs[4].b64[0], gprs[5].b64[0]); break;
	case 0x7a: break;
	default: Helpers::Panic("Unhandled SYSCALL $v1 = %xh\n", gprs[3].b64[0]);
	}
	return;
}
void EE::SetGsCrt(bool interlaced, int display_mode, bool frame) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) SetGsCrt [IGNORED]\n");
}
void EE::GsPutIMR(u64 value) {
	mem->gs->imr = value;
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) GsPutIMR(0x%016x);\n", value);
}
void EE::SetSyscall(int index, u32 address) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) SetSyscall(0x%x, 0x%08x)\n", index, address);
	CustomSyscall syscall;
	syscall.index = index;
	syscall.address = address;
	custom_syscalls.push_back(syscall);
}
void EE::InitMainThread(u32 gp, u32 stack, int stack_size, u32 args, int root) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) InitMainThread(0x%x, 0x%x, %d); [partially stubbed]\n", gp, stack, stack_size);
	main_thread_stack = ((s32)stack != -1) ? (stack + stack_size) : (0x2000000 - stack_size);
	gprs[2].b64[0] = main_thread_stack;
}
void EE::InitHeap(u32 heap, int heap_size) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) InitHeap(0x%x, %d);\n", heap, heap_size);
	gprs[2].b64[0] = (heap_size != -1) ? (heap + heap_size) : main_thread_stack;
}
void EE::CreateSema(u32 semaparam_ptr) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) CreateSema(0x%08x); [stubbed]\n", semaparam_ptr);
	gprs[2].b64[0] = ++sema_id;
}
void EE::DeleteSema(int sema_id) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) DeleteSema(%d); [stubbed]\n", sema_id);
}
void EE::WaitSema(int sema_id) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) WaitSema(%d); [stubbed]\n", sema_id);
}
void EE::SifSetDma(u32 transfer_ptr, int len) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) SifSetDma(0x%08x, %d); [stubbed]\n", transfer_ptr, len);
	gprs[2].b64[0] = 1;
}

void EE::Step() {
	mem->tmr1_stub++;
	mem->t0.step();
	mem->t1.step();
	mem->t2.step();
	mem->t3.step();

	if (mem->int1 && ((cop0r[12].b64[0] >> 11) & 1)) {
		if ((cop0r[12].b64[0] & 1) && ((cop0r[12].b64[0] >> 16) & 1) && !((cop0r[12].b64[0] >> 1) & 1) && !((cop0r[12].b64[0] >> 2) & 1)) {
			Exception(Exceptions::Interrupt, true);
			printf("[INTC] INT1 Fired\n");
			mem->int1 = false;
		}
	}

	if (mem->intc_stat & mem->intc_mask) {
		cop0r[13].b64[0] |= 1 << 10;
	} else {
		cop0r[13].b64[0] &= ~(1 << 10);
	}

	if	(
		   (cop0r[12].b64[0] & 1) 
		&& ((cop0r[12].b64[0] >> 16) & 1) 
		&& !((cop0r[12].b64[0] >> 1) & 1) 
		&& !((cop0r[12].b64[0] >> 2) & 1) 
		&& ((cop0r[12].b64[0] & (1 << 10)) && (cop0r[13].b64[0] & (1 << 10)))
		) {
		printf("[INTC] INT0\n");
		Exception(Exceptions::Interrupt, false);
	}

#ifdef BIOS_HLE
	if (pc == 0x1fc00000) {
		pc = pc_old + 4;
		gprs[31].b64[0] = ra_old;
	}
#endif

	cop0r[9].b64[0] += 2;
	u32 instr = mem->Read<u32>(pc);
	trace(Helpers::Log::EEd, "0x%08x ", pc);

	Instruction instruction;
	instruction.raw = instr;
	Execute(instruction);
	//if(branch_pc != std::nullopt) Helpers::Debug(Helpers::Log::EEd, "[delayed] ");
	if (mem->print_cnt == 512) {
		mem->print_cnt++;
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2autotests-master\\ps2autotests-master\\tests\\gs\\label.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_01\\demo1.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02a\\demo2a.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\ps2tut\\ps2tut\\ps2tut_02b\\demo2b.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\graph.elf");
		//branch_pc = std::nullopt;
		//traceb = true;
	}

	//if (pc == 0x8000688c) traceb = true;
}