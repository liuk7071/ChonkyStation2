#include "EE.h"

EE::EE(Memory* memptr) {
	mem = memptr;
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
		case JR: {
			branch_pc = gprs[instr.rs].b32[0];
			Helpers::Debug(Helpers::Log::EEd, "jr %s\n", gpr[instr.rs.Value()].c_str());
			break;
		}
		case SYSCALL: {
			Syscall(gprs[3].b64[0]);
			break;
		}
		case ADDU: {
			u32 result = gprs[instr.rs].b32[0] + gprs[instr.rt].b32[0];
			gprs[instr.rd].b64[0] = (u64)(s32)result;
			Helpers::Debug(Helpers::Log::EEd, "add %s, %s, %s\n", gpr[instr.rd.Value()], gpr[instr.rs.Value()], gpr[instr.rt.Value()]);
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
		default: 
			Helpers::Panic("Unimplemented SPECIAL instruction 0x%02x @ 0x%08x\n", instr.raw & 0x3f, pc);
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
	case ADDIU: {
		gprs[instr.rt].b64[0] = (u64)(gprs[instr.rs].b64[0] + (u64)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "addiu %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ANDI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] & (instr.imm));
		Helpers::Debug(Helpers::Log::EEd, "andi %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case ORI: {
		gprs[instr.rt].b64[0] = (gprs[instr.rs].b64[0] | (instr.imm));
		Helpers::Debug(Helpers::Log::EEd, "ori %s, %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), gpr[instr.rs.Value()].c_str(), instr.imm.Value());
		break;
	}
	case LUI: {
		gprs[instr.rt].b64[0] = (u64)(s32)(instr.imm << 16);
		Helpers::Debug(Helpers::Log::EEd, "lui %s, 0x%04x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value());
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
	case LB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = (s64)data;
		Helpers::Debug(Helpers::Log::EEd, "lb %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LW: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (lw)\n");
		}
		u32 data = mem->Read<u32>(address);
		gprs[instr.rt].b64[0] = (u64)(s32)data;
		Helpers::Debug(Helpers::Log::EEd, "lw %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case LBU: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		u8 data = mem->Read<u8>(address);
		gprs[instr.rt].b64[0] = (u64)(s8)data;
		Helpers::Debug(Helpers::Log::EEd, "lbu %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%02x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SB: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		Helpers::Debug(Helpers::Log::EEd, "sb, %s, 0x%04x(%s) ; mem[0x%08x] <- 0x%02x\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), address, gprs[instr.rt.Value()].b8[0]);
		mem->Write<u8>(address, gprs[instr.rt].b8[0]);
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
	case LD: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
			// TODO: Handle exceptions
			// Panic for now
			Helpers::Panic("Unhandled Address Error (ld)\n");
		}
		u64 data = mem->Read<u64>(address);
		gprs[instr.rt].b64[0] = data;
		Helpers::Debug(Helpers::Log::EEd, "ld %s, 0x%04x(%s) ; %s <- mem[0x%08x] (0x%04x)\n", gpr[instr.rt.Value()].c_str(), instr.imm.Value(), gpr[instr.rs.Value()].c_str(), gpr[instr.rt.Value()].c_str(), address, data);
		break;
	}
	case SD: {
		u32 address = (gprs[instr.rs].b32[0] + (u32)(s16)instr.imm);
		if (address & 3) {
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
	case 0x71: GsPutIMR(gprs[4].b64[0]); break;
	default: Helpers::Panic("Unhandled SYSCALL $v1 = %xh\n", gprs[3].b64[0]);
	}
	return;
}
void EE::SetGsCrt(bool interlaced, int display_mode, bool frame) {
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) SetGsCrt [IGNORED]\n");
}
void EE::GsPutIMR(u64 value) {
	mem->gs->IMR = value;
	Helpers::Debug(Helpers::Log::NOCOND, "(SYSCALL) GsPutIMR(0x%016x);\n", value);
}

void EE::Step() {
	// Eventually there will be more to this
	u32 instr = mem->Read<u32>(pc);
	Helpers::Debug(Helpers::Log::EEd, "0x%08x ", pc);

	Instruction instruction;
	instruction.raw = instr;
	Execute(instruction);
	if(branch_pc != std::nullopt)Helpers::Debug(Helpers::Log::EEd, "[delayed] ");
}