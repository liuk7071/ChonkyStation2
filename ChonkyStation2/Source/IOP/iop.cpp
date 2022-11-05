// This was ported from my PS1 emulator (ChonkyStation) CPU core

#include "iop.h"
#ifdef TEST_GTE
#include "tests.h"
#endif TEST_GTE
#include <iostream>

#ifdef log_cpu
#define debug_log(fmt, ...) if (debug) { \
printf((fmt), __VA_ARGS__); \
}
#else
#define debug_log
#endif

IOP::IOP(Memory* memptr) : mem(memptr) {
	pc = 0xbfc00000;
	next_instr = pc + 4;
	for (int i = 0x0; i < 32; i++) {
		regs[i] = 0;
	}

	COP0.regs[15] = 0x12;
}

inline void IOP::debug_warn(const char* fmt, ...) {
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
	std::va_list args;
	va_start(args, fmt);
	std::vprintf(fmt, args);
	va_end(args);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}
inline void IOP::debug_err(const char* fmt, ...) {
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	std::va_list args;
	va_start(args, fmt);
	std::vprintf(fmt, args);
	va_end(args);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}


void IOP::exception(exceptions exc) {
	uint32_t handler = 0;
	//if (delay) return;
	if (delay) COP0.regs[13] |= (1 << 31); else COP0.regs[13] &= ~(1 << 31);

	if (exc == 0x4 || exc == 0x5) {		// BadVAddr
		COP0.regs[8] = pc;
	}

	if (COP0.regs[12] & 0x400000) {
		handler = 0xbfc00180;
	}
	else {
		handler = 0x80000080;
	}

	uint32_t temp = COP0.regs[12] & 0x3f;
	COP0.regs[12] &= ~0x3f;
	COP0.regs[12] |= ((temp << 2) & 0x3f);
	COP0.regs[13] &= ~0xff;
	COP0.regs[13] |= (uint32_t(exc) << 2);
	COP0.regs[14] = pc;
	if (delay) COP0.regs[14] -= 4;
	pc = handler;
	delay = false;
}

void IOP::execute(uint32_t instr) {
	regs[0] = 0; // $zero

	uint8_t primary = instr >> 26;

	if (delay) {	// branch delay slot
		pc = jump - 4;
		delay = false;
	}

	// Quick and dirty hack. TODO: Replace this with a bitfield
	uint8_t rs = (instr >> 21) & 0x1f;
	uint8_t rd = (instr >> 11) & 0x1f;
	uint8_t rt = (instr >> 16) & 0x1f;
	int32_t signed_rs = int32_t(regs[rs]);
	uint16_t imm = instr & 0xffff;
	uint32_t sign_extended_imm = uint32_t(int16_t(imm));
	int32_t signed_sign_extended_imm = int32_t(uint32_t(int32_t(int16_t(imm))));	// ??????????? is this even needed
	uint8_t shift_imm = (instr >> 6) & 0x1f;
	uint32_t jump_imm = instr & 0x3ffffff;

	switch (primary) {
	case 0x00: {
		uint8_t secondary = instr & 0x3f;
		switch (secondary) {
		case 0x00: {
			uint32_t result = regs[rt] << shift_imm;
			regs[rd] = result;
			debug_log("sll %s, %s, 0x%.8x\n", reg[rd].c_str(), reg[rt].c_str(), shift_imm);
			break;
		}
		case 0x02: {
			regs[rd] = regs[rt] >> shift_imm;
			debug_log("srl 0x%.2X, %s, 0x%.8x\n", reg[rd].c_str(), reg[rt].c_str(), shift_imm);
			break;
		}
		case 0x03: {
			regs[rd] = int32_t(regs[rt]) >> shift_imm;
			debug_log("sra %s, %s, 0x%.8x\n", reg[rd].c_str(), reg[rt].c_str(), shift_imm);
			break;
		}
		case 0x04: {
			regs[rd] = regs[rt] << (regs[rs] & 0x1f);
			debug_log("sllv %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x06: {
			regs[rd] = regs[rt] >> (regs[rs] & 0x1f);
			debug_log("srlv %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x07: {
			regs[rd] = uint32_t(int32_t(regs[rt]) >> (regs[rs] & 0x1f));
			debug_log("srav %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x08: {
			uint32_t addr = regs[rs];
			if (addr & 3) {
				exception(exceptions::BadFetchAddr);
				return;
			}
			jump = addr;
			debug_log("jr %s\n", reg[rs].c_str());
			delay = true;
			break;
		}
		case 0x09: {
			uint32_t addr = regs[rs];
			regs[rd] = pc + 8;
			if (addr & 3) {
				exception(exceptions::BadFetchAddr);
				return;
			}
			jump = addr;
			debug_log("jalr %s\n", reg[rs].c_str());
			delay = true;
			break;
		}
		case 0x0C: {
			debug_log("syscall\n");
			exception(exceptions::SysCall);
			return;
		}
		case 0x0D: {
			debug_log("break\n");
			exception(exceptions::Break);
			return;
		}
		case 0x10: {
			regs[rd] = hi;
			debug_log("mfhi %s\n", reg[rd].c_str());
			break;
		}
		case 0x11: {
			hi = regs[rs];
			debug_log("mthi %s\n", reg[rs].c_str());
			break;
		}
		case 0x12: {
			regs[rd] = lo;
			debug_log("mflo %s\n", reg[rd].c_str());
			break;
		}
		case 0x13: {
			lo = regs[rs];
			debug_log("mtlo %s\n", reg[rs].c_str());
			break;
		}
		case 0x18: {
			int64_t x = int64_t(int32_t(regs[rs]));
			int64_t y = int64_t(int32_t(regs[rt]));
			uint64_t result = uint64_t(x * y);

			hi = uint32_t((result >> 32) & 0xffffffff);
			lo = uint32_t(result & 0xffffffff);
			debug_log("mult %s, %s", reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x19: {
			uint64_t x = uint64_t(regs[rs]);
			uint64_t y = uint64_t(regs[rt]);
			uint64_t result = x * y;

			hi = uint32_t(result >> 32);
			lo = uint32_t(result);
			debug_log("multu %s, %s", reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x1A: {
			int32_t n = int32_t(regs[rs]);
			int32_t d = int32_t(regs[rt]);

			if (d == 0) {
				hi = uint32_t(n);
				if (n >= 0) {
					lo = 0xffffffff;
				}
				else {
					lo = 1;
				}
				break;
			}
			if (uint32_t(n) == 0x80000000 && d == -1) {
				hi = 0;
				lo = 0x80000000;
				break;
			}
			hi = uint32_t(n % d);
			lo = uint32_t(n / d);
			debug_log("div %s, %s\n", reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x1B: {
			if (regs[rt] == 0) {
				hi = regs[rs];
				lo = 0xffffffff;
				break;
			}
			hi = regs[rs] % regs[rt];
			lo = regs[rs] / regs[rt];
			debug_log("divu %s, %s\n", reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x20: {
			debug_log("add %s, %s, %s", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			uint32_t result = regs[rs] + regs[rt];

			if (((regs[rs] >> 31) == (regs[rt] >> 31)) && ((regs[rs] >> 31) != (result >> 31))) {
				debug_log(" add exception");
				exception(exceptions::Overflow);
				return;
			}
			debug_log("\n");
			regs[rd] = result;
			break;
		}
		case 0x21: {
			regs[rd] = regs[rs] + regs[rt];
			debug_log("addu %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x22: {
			uint32_t result = regs[rs] - regs[rt];
			debug_log("sub %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			if (((regs[rs] ^ result) & (~regs[rt] ^ result)) >> 31) { // overflow
				exception(exceptions::Overflow);
				return;
			}
			regs[rd] = result;
			break;
		}
		case 0x23: {
			regs[rd] = regs[rs] - regs[rt];
			debug_log("subu %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x24: {
			regs[rd] = regs[rs] & regs[rt];
			debug_log("and %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x25: {
			regs[rd] = regs[rs] | regs[rt];
			debug_log("or %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x26: {
			regs[rd] = regs[rs] ^ regs[rt];
			debug_log("xor %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x27: {
			regs[rd] = ~(regs[rs] | regs[rt]);
			debug_log("nor %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			break;
		}
		case 0x2A: {
			debug_log("slt %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			regs[rd] = int32_t(regs[rs]) < int32_t(regs[rt]);
			break;
		}
		case 0x2B: {
			debug_log("sltu %s, %s, %s\n", reg[rd].c_str(), reg[rs].c_str(), reg[rt].c_str());
			regs[rd] = regs[rs] < regs[rt];
			break;
		}

		default:
			debug_err("\nUnimplemented subinstruction: 0x%x", secondary);
			exit(0);

		}
		break;
	}
	case 0x01: {
		auto bit16 = (instr >> 16) & 1;
		bool link = ((instr >> 17) & 0xF) == 8;
		if (bit16 == 0) {		// bltz
			if (link) regs[0x1f] = pc + 8; // check if link (bltzal)
			debug_log("BxxZ %s, 0x%.4x", reg[rs].c_str(), sign_extended_imm);
			if (signed_rs < 0) {
				jump = (pc + 4) + (sign_extended_imm << 2);
				delay = true;
				debug_log(" branched\n");
			}
			else { debug_log("\n"); }
			break;
		}
		else {		// bgez
			debug_log("BxxZ %s, 0x%.4x", reg[rs].c_str(), sign_extended_imm);
			if (link) regs[0x1f] = pc + 8; // check if link (bgezal)
			if (signed_rs >= 0) {
				jump = (pc + 4) + (sign_extended_imm << 2);
				delay = true;
				debug_log(" branched\n");
			}
			else { debug_log("\n"); }
			break;
		}
		break;
	}
	case 0x02: {
		jump = (pc & 0xf0000000) | (jump_imm << 2);
		debug_log("j 0x%.8x\n", jump_imm);
		delay = true;
		break;
	}
	case 0x03: {
		jump = (pc & 0xf0000000) | (jump_imm << 2);
		debug_log("jal 0x%.8x\n", jump_imm);
		regs[0x1f] = pc + 8;
		delay = true;
		break;
	}
	case 0x04: {
		debug_log("beq %s, %s, 0x%.4x", reg[rs].c_str(), reg[rt].c_str(), sign_extended_imm);
		if (regs[rs] == regs[rt]) {
			jump = (pc + 4) + (sign_extended_imm << 2);
			delay = true;
			debug_log(" branched\n");
		}
		else { debug_log("\n"); }
		break;
	}
	case 0x05: {
		debug_log("bne %s, %s, 0x%.4x", reg[rs].c_str(), reg[rt].c_str(), sign_extended_imm);
		if (regs[rs] != regs[rt]) {
			jump = (pc + 4) + (sign_extended_imm << 2);
			delay = true;
			debug_log(" branched\n");
		}
		else { debug_log("\n"); }
		break;
	}
	case 0x06: {
		debug_log("blez %s, 0x%.4x", reg[rs].c_str(), sign_extended_imm);
		if (signed_rs <= 0) {
			jump = (pc + 4) + (sign_extended_imm << 2);
			delay = true;
			debug_log(" branched\n");
		}
		else { debug_log("\n"); }
		break;
	}
	case 0x07: {
		debug_log("bgtz %s, 0x%.4x", reg[rs].c_str(), sign_extended_imm);
		if (signed_rs > 0) {
			jump = (pc + 4) + (sign_extended_imm << 2);
			delay = true;
			debug_log(" branched\n");
		}
		else { debug_log("\n"); }
		break;
	}
	case 0x08: {
		debug_log("addi %s, %s, 0x%.4x", reg[rs].c_str(), reg[rt].c_str(), imm);
		uint32_t result = regs[rs] + sign_extended_imm;
		if (((regs[rs] >> 31) == (sign_extended_imm >> 31)) && ((regs[rs] >> 31) != (result >> 31))) {
			debug_log(" addi exception");
			exception(exceptions::Overflow);
			return;
		}
		debug_log("\n");
		regs[rt] = result;
		break;
	}
	case 0x09: {
		regs[rt] = regs[rs] + sign_extended_imm;
		debug_log("addiu %s, %s, 0x%.4x\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		break;
	}
	case 0x0A: {
		regs[rt] = 0;
		if (signed_rs < (int32_t)sign_extended_imm)
			regs[rt] = 1;
		debug_log("slti %s, %s, 0x%.4X\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		break;
	}
	case 0x0B: {
		regs[rt] = regs[rs] < sign_extended_imm;
		debug_log("sltiu %s, %s, 0x%.4X\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		break;
	}
	case 0x0C: {
		debug_log("andi %s, %s, 0x%.4x\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		regs[rt] = (regs[rs] & imm);
		break;
	}
	case 0x0D: {
		debug_log("ori %s, %s, 0x%.4x\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		regs[rt] = (regs[rs] | imm);
		break;
	}
	case 0x0E: {
		debug_log("xori %s, %s, 0x%.4x\n", reg[rs].c_str(), reg[rt].c_str(), imm);
		regs[rt] = (regs[rs] ^ imm);
		break;
	}
	case 0x0F: {
		debug_log("lui %s, 0x%.4x\n", reg[rt].c_str(), imm);
		regs[rt] = imm << 16;
		break;
	}
	case 0x10: {
		uint8_t cop_instr = instr >> 21;
		switch (cop_instr) {
		case(0b00000): { // mfc0
			regs[rt] = COP0.regs[rd];
			debug_log("mfc0 %s, %s\n", reg[rd].c_str(), reg[rt].c_str());
			break;
		}
		case(0b00100): { // mtc0
			COP0.regs[rd] = regs[rt];
			debug_log("mtc0 %s, %s\n", reg[rd].c_str(), reg[rt].c_str());
			break;
		}
		case(0b10000): { // rfe
			if ((instr & 0x3f) != 0b010000) {
				printf("Invalid RFE");
				exit(0);
			}
			debug_log("rfe");
			//auto mode = COP0.regs[12] & 0x3f;
			//COP0.regs[12] &= ~0x3f;
			//COP0.regs[12] |= mode >> 2;
			COP0.regs[12] = (COP0.regs[12] & 0xfffffff0) | ((COP0.regs[12] & 0x3c) >> 2);
			break;
		}
		default:
			printf("Unknown coprocessor instruction: 0x%.2x", cop_instr);
			exit(0);
		}
		break;
	}
	case 0x12: {
		printf("gte\n");
		exit(0);
		//GTE.execute(instr, regs);
		break;
	}
	case 0x20: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		if ((COP0.regs[12] & 0x10000) == 1) {
			debug_log(" cache isolated, ignoring load\n");
			break;
		}

		uint8_t byte = mem->IOPRead<u8>(addr);
		regs[rt] = int32_t(int16_t(int8_t(byte)));
		debug_log("lb %s, 0x%.4x(%s)\n", reg[rt].c_str(), imm, reg[rs].c_str());
		break;
	}
	case 0x21: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("lh %s, 0x%.4x(%s)\n", reg[rt].c_str(), imm, reg[rs].c_str());
		if ((COP0.regs[12] & 0x10000) == 0) {
			int16_t data = int16_t(mem->IOPRead<u16>(addr));
			regs[rt] = uint32_t(int32_t(data));
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring load\n");
		}
		break;
	}
	case 0x22: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		uint32_t aligned_addr = addr & ~3;
		uint32_t aligned_word = mem->IOPRead<u32>(aligned_addr);
		debug_log("lwl %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		switch (addr & 3) {
		case 0:
			regs[rt] = (regs[rt] & 0x00ffffff) | (aligned_word << 24);
			break;
		case 1:
			regs[rt] = (regs[rt] & 0x0000ffff) | (aligned_word << 16);
			break;
		case 2:
			regs[rt] = (regs[rt] & 0x000000ff) | (aligned_word << 8);
			break;
		case 3:
			regs[rt] = (regs[rt] & 0x00000000) | (aligned_word << 0);
			break;
		}
		break;
	}
	case 0x23: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("lw %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		uint32_t value = mem->IOPRead<u32>(addr);
		debug_log(" ; addr = 0x%08x, val = 0x%08x", addr, value);
		if ((COP0.regs[12] & 0x10000) == 0) {
			regs[rt] = value;
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring load\n");
		}
		break;
	}
	case 0x24: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("lbu %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		if ((COP0.regs[12] & 0x10000) == 0) {
			uint8_t byte = mem->IOPRead<u32>(addr);
			regs[rt] = uint32_t(byte);
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring load\n");
		}
		break;
	}
	case 0x25: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("lhu %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		if ((COP0.regs[12] & 0x10000) == 0) {
			uint16_t data = mem->IOPRead<u16>(addr);
			regs[rt] = uint32_t(data);
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring load\n");
		}
		break;
	}
	case 0x26: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		uint32_t aligned_addr = addr & ~3;
		uint32_t aligned_word = mem->IOPRead<u32>(aligned_addr);
		debug_log("lwr %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		switch (addr & 3) {
		case 0:
			regs[rt] = (regs[rt] & 0x00000000) | (aligned_word >> 0);
			break;
		case 1:
			regs[rt] = (regs[rt] & 0xff000000) | (aligned_word >> 8);
			break;
		case 2:
			regs[rt] = (regs[rt] & 0xffff0000) | (aligned_word >> 16);
			break;
		case 3:
			regs[rt] = (regs[rt] & 0xffffff00) | (aligned_word >> 24);
			break;
		}
		break;
	}
	case 0x28: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("sb %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		if ((COP0.regs[12] & 0x10000) == 0) {
			mem->IOPWrite<u8>(addr, uint8_t(regs[rt]));
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring write\n");
		}
		break;
	}
	case 0x29: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		if (addr & 1) {
			exception(exceptions::BadStoreAddr);
			return;
		}
		debug_log("sh %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		if ((COP0.regs[12] & 0x10000) == 0) {
			mem->IOPWrite<u16>(addr, uint16_t(regs[rt]));
			debug_log("\n");
		}
		else {
			debug_log(" cache isolated, ignoring write\n");
		}
		break;
	}
	case 0x2A: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		uint32_t aligned_addr = addr & ~3;

		uint32_t mem_ = mem->IOPRead<u32>(aligned_addr);
		uint32_t val = 0;

		switch (addr & 3) {
		case 0:
			val = (mem_ & 0xffffff00) | (regs[rt] >> 24);
			break;
		case 1:
			val = (mem_ & 0xffff0000) | (regs[rt] >> 16);
			break;
		case 2:
			val = (mem_ & 0xff000000) | (regs[rt] >> 8);
			break;
		case 3:
			val = (mem_ & 0x00000000) | (regs[rt] >> 0);
			break;
		}

		mem->IOPWrite<u32>(aligned_addr, val);
		debug_log("swl %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		break;
	}
	case 0x2B: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		debug_log("sw %s, 0x%.4x(%s) ; %s = 0x%.8x\n", reg[rt].c_str(), imm, reg[rs].c_str(), reg[rs].c_str(), regs[rs]);
		if (addr & 3) {
			exception(exceptions::BadStoreAddr);
			return;
		}

		if ((COP0.regs[12] & 0x10000) == 0) {
			mem->IOPWrite<u32>(addr, regs[rt]);
			debug_log("\n");
		}
		break;
	}
	case 0x2E: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		uint32_t aligned_addr = addr & ~3;

		uint32_t mem_ = mem->IOPRead<u32>(aligned_addr);
		uint32_t val = 0;

		switch (addr & 3) {
		case 0:
			val = (mem_ & 0x00000000) | (regs[rt] << 0);
			break;
		case 1:
			val = (mem_ & 0x000000ff) | (regs[rt] << 8);
			break;
		case 2:
			val = (mem_ & 0x0000ffff) | (regs[rt] << 16);
			break;
		case 3:
			val = (mem_ & 0x00ffffff) | (regs[rt] << 24);
			break;
		}

		mem->IOPWrite<u32>(aligned_addr, val);
		debug_log("swr %s, 0x%.4x(%s)", reg[rt].c_str(), imm, reg[rs].c_str());
		break;
	}
	case 0x32: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		printf("gte");
		exit(0);
		break;
	}
	case 0x3A: {
		uint32_t addr = regs[rs] + sign_extended_imm;
		printf("gte");
		exit(0);
		break;
	}
	default:
		debug_err("\nUnimplemented instruction: 0x%x @ 0x%08x", primary, pc);
		exit(0);
	}
	pc += 4;
}

void IOP::step() {
	if (mem->iop_i_stat & mem->iop_i_mask) {
		COP0.regs[13] |= (1 << 10);
		if ((COP0.regs[12] & 1) && (COP0.regs[12] & (1 << 10))) {
			printf("[INTC] (IOP) Interrupt fired\n");
			exception(exceptions::INT);
		}
	}
	const auto instr = mem->IOPRead<u32>(pc);
#ifdef log_cpu
	debug_log("0x%.8X | 0x%.8X: ", pc, instr);
#endif

	// From ps2tek
	if (pc == 0x12C48 || pc == 0x1420C || pc == 0x1430C) {
		uint32_t pointer = regs[5];
		uint32_t text_size = regs[6];
		while (text_size) {
			auto c = (char)mem->iop_ram[pointer & 0x1FFFFF];
			printf("%c", c);

			pointer++;
			text_size--;
		}
	}

	execute(instr);
}