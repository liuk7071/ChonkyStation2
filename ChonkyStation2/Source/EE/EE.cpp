#include "EE.h"

EE::EE(Memory* memptr) {
	mem = memptr;
	cop0r[15].b64[0] = 0x69;
	mem->pc = &pc;
}

void EE::Exception(unsigned int exception) {
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
	pc = 0x80000180;
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
			Helpers::Debug(Helpers::Log::EEd, "sll %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SRL: {
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] >> instr.sa);
			Helpers::Debug(Helpers::Log::EEd, "srl %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SRA: {
			gprs[instr.rd].b64[0] = (u64)(s32)((s32)gprs[instr.rt].b32[0] >> instr.sa);
			Helpers::Debug(Helpers::Log::EEd, "sra %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case SLLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = (u64)(s32)(gprs[instr.rt].b32[0] << shift);
			Helpers::Debug(Helpers::Log::EEd, "sllv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SRAV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = (u64)(s32)((s32)gprs[instr.rt].b32[0] >> shift);
			Helpers::Debug(Helpers::Log::EEd, "srav %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case JR: {
			branch_pc = gprs[instr.rs].b32[0];
			Helpers::Debug(Helpers::Log::EEd, "jr %s\n", gpr[instr.rs.Value()].c_str());
			break;
		}
		case JALR: {
			branch_pc = gprs[instr.rs].b32[0];
			gprs[instr.rd].b64[0] = (pc + 8);
			Helpers::Debug(Helpers::Log::EEd, "jalr %s %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str());
			break;
		}
		case MOVZ: {
			if (gprs[instr.rt].b64[0] == 0) gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0];
			Helpers::Debug(Helpers::Log::EEd, "movz %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MOVN: {
			if (gprs[instr.rt].b64[0] != 0) gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0];
			Helpers::Debug(Helpers::Log::EEd, "movn %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SYSCALL: {
			//Syscall(gprs[3].b64[0]);	// HLE
			Exception(Exceptions::ExSYSCALL);
			return;
		}
		case SYNC: {
			Helpers::Debug(Helpers::Log::EEd, "sync\n");
			break;
		}
		case MFHI: {
			gprs[instr.rd].b64[0] = hi.b64[0];
			Helpers::Debug(Helpers::Log::EEd, "mfhi %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MFLO: {
			gprs[instr.rd].b64[0] = lo.b64[0];
			Helpers::Debug(Helpers::Log::EEd, "mflo %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case DSLLV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = gprs[instr.rt].b64[0] << shift;
			Helpers::Debug(Helpers::Log::EEd, "dsllv %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSRAV: {
			const int shift = gprs[instr.rs].b64[0] & 0x3f;
			gprs[instr.rd].b64[0] = (s64)gprs[instr.rt].b64[0] >> shift;
			Helpers::Debug(Helpers::Log::EEd, "dsrav %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case MULT: {
			s64 result = (s32)gprs[instr.rs].b32[0] * (s32)gprs[instr.rt].b32[0];
			lo.b64[0] = (u64)(s32)(result & 0xffffffff);
			hi.b64[0] = (u64)(s32)((result >> 32) & 0xffffffff);
			Helpers::Debug(Helpers::Log::EEd, "mult %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
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
			Helpers::Debug(Helpers::Log::EEd, "div %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
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
			Helpers::Debug(Helpers::Log::EEd, "divu %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case ADD: {	// TODO: Overflow
			u32 result = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			Helpers::Debug(Helpers::Log::EEd, "add %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case ADDU: {
			u32 result = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			Helpers::Debug(Helpers::Log::EEd, "addu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SUB: {	// TODO: Overflow
			u64 result = gprs[instr.rs].b64[0] - gprs[instr.rt].b64[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			Helpers::Debug(Helpers::Log::EEd, "sub %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SUBU: {
			u64 result = gprs[instr.rs].b64[0] - gprs[instr.rt].b64[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			Helpers::Debug(Helpers::Log::EEd, "subu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case AND: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] & gprs[instr.rt].b64[0]);
			Helpers::Debug(Helpers::Log::EEd, "and %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case OR: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
			Helpers::Debug(Helpers::Log::EEd, "or %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case NOR: {
			gprs[instr.rd].b64[0] = ~(gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
			Helpers::Debug(Helpers::Log::EEd, "nor %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SLT: {
			gprs[instr.rd].b64[0] = ((s64)gprs[instr.rs].b64[0] < (s64)gprs[instr.rt].b64[0]);
			Helpers::Debug(Helpers::Log::EEd, "slt %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case SLTU: {
			gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] < gprs[instr.rt].b64[0]);
			Helpers::Debug(Helpers::Log::EEd, "sltu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DADDU: {
			gprs[instr.rd].b64[0] = gprs[instr.rs].b64[0] + gprs[instr.rt].b64[0];
			Helpers::Debug(Helpers::Log::EEd, "daddu %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
			break;
		}
		case DSLL: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] << instr.sa);
			Helpers::Debug(Helpers::Log::EEd, "dsll %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRL: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] >> instr.sa);
			Helpers::Debug(Helpers::Log::EEd, "dsrl %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSLL32: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] << (instr.sa + 32));
			Helpers::Debug(Helpers::Log::EEd, "dsll32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRL32: {
			gprs[instr.rd].b64[0] = (gprs[instr.rt].b64[0] >> (instr.sa + 32));
			Helpers::Debug(Helpers::Log::EEd, "dsrl32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
			break;
		}
		case DSRA32: {
			gprs[instr.rd].b64[0] = ((s64)gprs[instr.rt].b64[0] >> (instr.sa + 32));
			Helpers::Debug(Helpers::Log::EEd, "dsra32 %s, %s, %d\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rt.Value()].c_str(), instr.sa.Value());
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
			Helpers::Debug(Helpers::Log::EEd, "bltz %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
			break;
		}
		case BGEZ: {
			u32 offset = instr.imm;
			if ((s64)gprs[instr.rs].b64[0] >= 0) {
				branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
			}
			Helpers::Debug(Helpers::Log::EEd, "bgez %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
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
			Helpers::Debug(Helpers::Log::EEd, "bltzl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
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
			Helpers::Debug(Helpers::Log::EEd, "bgezl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
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
		Helpers::Debug(Helpers::Log::EEd, "j 0x%x\n", offset);
		break;
	}
	case JAL: {
		u32 offset = instr.jump_imm;
		branch_pc = (((pc + 4) & 0xf0000000) | (offset << 2));
		gprs[31].b64[0] = (pc + 8);
		Helpers::Debug(Helpers::Log::EEd, "jal 0x%x\n", offset);
		break;
	}
	case BEQ: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] == gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		Helpers::Debug(Helpers::Log::EEd, "beq %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BNE: {
		u32 offset = instr.imm;
		if (gprs[instr.rs].b64[0] != gprs[instr.rt].b64[0]) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		Helpers::Debug(Helpers::Log::EEd, "bne %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
		break;
	}
	case BLEZ: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] <= 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		Helpers::Debug(Helpers::Log::EEd, "blez %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case BGTZ: {
		u32 offset = instr.imm;
		if ((s64)gprs[instr.rs].b64[0] > 0) {
			branch_pc = ((pc + 4) + (u32)(s16)(offset << 2));
		}
		Helpers::Debug(Helpers::Log::EEd, "bgtz %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case ADDI: {	// TODO: Overflow
		gprs[instr.rt].b64[0] = (s32)(gprs[instr.rs].b64[0] + (u64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "addi %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ADDIU: {
		gprs[instr.rt].b64[0] = (s32)(gprs[instr.rs].b64[0] + (u64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "addiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case SLTI: {
		gprs[instr.rt].b64[0] = ((s64)gprs[instr.rs].b64[0] < (s64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "slti %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case SLTIU: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] < (s64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "sltiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ANDI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] & instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "andi %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ORI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] | instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "ori %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case XORI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] ^ instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "xori %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case LUI: {
		gprs[instr.rt].b64[0] = (u64)(s32)(instr.imm << 16);
		Helpers::Debug(Helpers::Log::EEd, "lui %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value());
		break;
	}
	case COP0: {
		switch ((instr.raw >> 21) & 0x1f) {
		case MFC0: {
			gprs[instr.rt].b64[0] = cop0r[instr.rd].b64[0];
			Helpers::Debug(Helpers::Log::EEd, "mfc0 %s, cop0r%d\n", gpr[instr.rt.Value()].c_str(), instr.rd.Value());
			break;
		}
		case MTC0: {
			cop0r[instr.rd].b64[0] = gprs[instr.rt].b64[0];
			Helpers::Debug(Helpers::Log::EEd, "mtc0 %s, cop0r%d\n", gpr[instr.rt.Value()].c_str(), instr.rd.Value());
			break;
		}
		case TLB: {
			switch (instr.raw & 0x3f) {
			case TLBWI: {
				Helpers::Debug(Helpers::Log::EEd, "tlbwi\n");
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
				Helpers::Debug(Helpers::Log::EEd, "eret\n");
				return;
			}
			case EI: {
				// TODO: Don't have interrupts
				Helpers::Debug(Helpers::Log::EEd, "ei\n");
				break;
			}
			case DI: {
				// TODO: Don't have interrupts
				Helpers::Debug(Helpers::Log::EEd, "di\n");
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
	case COP2: {
		printf("Unimplemented COP2 instruction 0x%08x\n", instr.raw);
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
		Helpers::Debug(Helpers::Log::EEd, "beql %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
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
		Helpers::Debug(Helpers::Log::EEd, "bnel %s, %s, 0x%04x\n", gpr[instr.rs.Value()], gpr[instr.rt.Value()], offset);
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
		Helpers::Debug(Helpers::Log::EEd, "blezl %s, 0x%04x\n", gpr[instr.rs.Value()], offset);
		break;
	}
	case DADDIU: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] + (u64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "daddiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case MMI: {
		switch (instr.raw & 0x3f) {
		case MFLO1: {
			gprs[instr.rd].b64[0] = lo.b64[1];
			Helpers::Debug(Helpers::Log::EEd, "mflo1 %s\n", gpr[instr.rd.Value()].c_str());
			break;
		}
		case MULT1: {
			s64 result = (s32)gprs[instr.rs].b32[0] * (s32)gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)(result & 0xffffffff);
			lo.b64[1] = (u64)(s32)(result & 0xffffffff);
			hi.b64[1] = (u64)(s32)((result >> 32) & 0xffffffff);
			Helpers::Debug(Helpers::Log::EEd, "mult1 %s %s %s\n", gpr[instr.rd.Value()].c_str(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
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
			Helpers::Debug(Helpers::Log::EEd, "divu1 %s %s\n", gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str());
			break;
		}
		case MMI3: {
			switch ((instr.raw >> 6) & 0x1f) {
			case POR: {
				gprs[instr.rd].b64[0] = (gprs[instr.rs].b64[0] | gprs[instr.rt].b64[0]);
				gprs[instr.rd].b64[1] = (gprs[instr.rs].b64[1] | gprs[instr.rt].b64[1]);
				Helpers::Debug(Helpers::Log::EEd, "por %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
				break;
			}
			default:
				Helpers::Panic("Unimplemented MMI3 instruction 0x%02x @ 0x%08x\n", (instr.raw >> 6) & 0x1f, pc);
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
		Helpers::Debug(Helpers::Log::EEd, "lq %s, 0x%04x(%s)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SQ: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		mem->Write<u64>(address, gprs[instr.rt].b64[0]);
		mem->Write<u64>(address + 8, gprs[instr.rt].b64[1]);
		Helpers::Debug(Helpers::Log::EEd, "sq, %s, 0x%04x(%s)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		break;
	}
	case LB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = (s8)data;
		Helpers::Debug(Helpers::Log::EEd, "lb %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LH: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u16 data = mem->Read<u16>(address);
		gprs[instr.rt].b64[0] = (s16)data;
		Helpers::Debug(Helpers::Log::EEd, "lh %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
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
		Helpers::Debug(Helpers::Log::EEd, "lw %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LBU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = data;
		Helpers::Debug(Helpers::Log::EEd, "lbu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LHU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u16 data = mem->Read<u16>(address);
		gprs[instr.rt].b64[0] = data;
		Helpers::Debug(Helpers::Log::EEd, "lhu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}

	case LWU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u32 data = mem->Read<u32>(address);
		gprs[instr.rt].b64[0] = data;
		Helpers::Debug(Helpers::Log::EEd, "lwu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "sb, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%02x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b8[0]);
		mem->Write<u8>(address, gprs[instr.rt].b8[0]);
		break;
	}
	case SH: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "sh, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b8[0]);
		mem->Write<u16>(address, gprs[instr.rt].b16[0]);
		break;
	}
	case SW: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (sw)\n");
		}
		mem->Write<u32>(address, gprs[instr.rt].b32[0]);
		Helpers::Debug(Helpers::Log::EEd, "sw, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		break;
	}
	case CACHE: {
		Helpers::Debug(Helpers::Log::EEd, "cache\n");
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
		Helpers::Debug(Helpers::Log::EEd, "ld %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SWC1: {
		Helpers::Debug(Helpers::Log::EEd, "swc1\n");
		break;
	}
	case SD: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 7) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (sw)\n");
		}
		mem->Write<u64>(address, gprs[instr.rt].b64[0]);
		Helpers::Debug(Helpers::Log::EEd, "sd, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b32[0]);
		break;
	}
	default:
		Helpers::Panic("Unimplemented instruction 0x%02x @ 0x%08x\n", (instr.raw >> 26) & 0x3f, pc);
	}
	pc += 4;
	
}

// HLE SYSCALLs
void EE::Syscall(u64 v1) {
	switch (v1) {
	case 0x02: SetGsCrt(gprs[4].b64[0], gprs[5].b64[0], gprs[6].b64[0]); break;
	case 0x12: Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) AddDmacHandler [IGNORED]\n"); break;
	case 0x16: gprs[2].b64[0] = 1; Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) _EnableDmac [IGNORED]\n"); break;
	case 0x3C: InitMainThread(gprs[4].b64[0], gprs[5].b64[0], gprs[6].b64[0], gprs[7].b64[0], gprs[8].b64[0]); break;
	case 0x3D: InitHeap(gprs[4].b64[0], gprs[5].b64[0]); break;
	case 0x64: Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) FlushCache [IGNORED]\n"); break;
	case 0x71: GsPutIMR(gprs[4].b64[0]); break;
	default: printf("Unhandled SYSCALL $v1 = %xh\n", gprs[3].b64[0]);
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
void EE::InitMainThread(u32 gp, u32 stack, int stack_size, u32 args, int root) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) InitMainThread(0x%x, 0x%x, %d); [partially stubbed]\n", gp, stack, stack_size);
	main_thread_stack = ((s32)stack != -1) ? (stack + stack_size) : (0x2000000 - stack_size);
	gprs[2].b64[0] = main_thread_stack;
}
void EE::InitHeap(u32 heap, int heap_size) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) InitHeap(0x%x, %d);\n", heap, heap_size);
	gprs[2].b64[0] = (heap_size != -1) ? (heap + heap_size) : main_thread_stack;
}

void EE::Step() {
	// Eventually there will be more to this
	cop0r[9].b64[0] += 2;
	u32 instr = mem->Read<u32>(pc);
	Helpers::Debug(Helpers::Log::EEd, "0x%08x ", pc);

	Instruction instruction;
	instruction.raw = instr;
	Execute(instruction);
	//if(branch_pc != std::nullopt) Helpers::Debug(Helpers::Log::EEd, "[delayed] ");
	if (pc == 0x80001000) {
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\3stars\\3stars\\3stars.elf");
		//pc = mem->LoadELF("C:\\Users\\zacse\\Downloads\\OSDSYS");
		//branch_pc = std::nullopt;
	}
}