#include "vu.h"

void VU::ExecVU0Macro(Instruction instr) {
	vf[0][x] = 0.0f;
	vf[0][y] = 0.0f;
	vf[0][z] = 0.0f;
	vf[0][w] = 1.0f;

	vi[0] = 0;
	switch (instr.rs) {
	case 0x10:
	case 0x18:	
	case 0x1e:
	case 0x1f: {	// Special1
		switch (instr.raw & 0x3f) {
		case SPECIAL1::VMADDx:
		case SPECIAL1::VMADDy:
		case SPECIAL1::VMADDz:
		case SPECIAL1::VMADDw:
			maddbc(instr); break;
		case SPECIAL1::VMULq:   mulq(instr);   break;
		case SPECIAL1::VIADD:   iadd(instr);   break;
		case SPECIAL1::VSUB:    sub(instr);    break;
		case 0x3c:
		case 0x3d:
		case 0x3e:
		case 0x3f: {	// Special2
			auto flo = instr.raw & 3;
			auto fhi = instr.sa;
			u8 opc = flo | (fhi * 4);
			switch (opc) {
			case SPECIAL2::VMADDAx:
			case SPECIAL2::VMADDAy: 
			case SPECIAL2::VMADDAz:
			case SPECIAL2::VMADDAw:
				maddabc(instr); break;
			case SPECIAL2::VMULAx:
			case SPECIAL2::VMULAw:
				mulabc(instr);   break;
			case SPECIAL2::VCLIPw: /* unimplemented */ break;
			case SPECIAL2::VSQI:    sqi(instr);      break;
			case SPECIAL2::VDIV:	div(instr);		 break;
			case SPECIAL2::VWAITQ:	break;
			case SPECIAL2::VISWR:   iswr(instr);     break;
			//default:
				//Helpers::Panic("Unimplemented VU0 macro SPECIAL2 instruction 0x%02x\n", opc);
			}
			break;
		}
		//default:
			//Helpers::Panic("Unimplemented VU0 macro SPECIAL1 instruction 0x%02x\n", instr.raw & 0x3f);
		}
		break;
	}
	//default:
		//Helpers::Panic("Unimplemented VU0 macro instruction 0x%02x\n", instr.rs.Value());
	}
}

void VU::Write32(u32 addr, u32 data) {
	if (addr >= 0x4000 && (addr <= 0x41f0)) {
		printf("Write VU1 floating register\n");
	}
	else if (addr >= 0x4200 && (addr <= 0x42f0)) {
		printf("Write VU1 integer register\n");
	}
	else {
		vu0_data_mem[addr] = data;
	}
}

bool VU::CheckDest(u8 dest, int component) {
	return (dest & (1 << (3 - component)));
}

// Instructions
void VU::maddbc(Instruction instr) {
	const auto bc = instr.raw & 3;
	const auto broadcasted = vf[instr.ft][bc];

	if (CheckDest(instr.dest, x)) vf[instr.fd][x] = vf[instr.fs][x] * broadcasted + acc[x];
	if (CheckDest(instr.dest, y)) vf[instr.fd][y] = vf[instr.fs][y] * broadcasted + acc[y];
	if (CheckDest(instr.dest, z)) vf[instr.fd][z] = vf[instr.fs][z] * broadcasted + acc[z];
	if (CheckDest(instr.dest, w)) vf[instr.fd][w] = vf[instr.fs][w] * broadcasted + acc[w];
}

void VU::mulq(Instruction instr) {
	if (CheckDest(instr.dest, x)) vf[instr.fd][x] = vf[instr.fs][x] * q;
	if (CheckDest(instr.dest, y)) vf[instr.fd][y] = vf[instr.fs][y] * q;
	if (CheckDest(instr.dest, z)) vf[instr.fd][z] = vf[instr.fs][z] * q;
	if (CheckDest(instr.dest, w)) vf[instr.fd][w] = vf[instr.fs][w] * q;
}

void VU::iadd(Instruction instr) {
	vi[instr.id] = vi[instr.is] + vi[instr.it];
}

void VU::sub(Instruction instr) {
	for (int i = 0; i < 4; i++) {
		if (CheckDest(instr.dest, i)) vf[instr.fd][i] = vf[instr.fs][i] - vf[instr.ft][i];
	}
}

void VU::maddabc(Instruction instr) {
	const auto bc = instr.raw & 3;

	if (CheckDest(instr.dest, x)) acc[x] = vf[instr.fs][x] * vf[instr.ft][bc] + acc[x];
	if (CheckDest(instr.dest, y)) acc[y] = vf[instr.fs][y] * vf[instr.ft][bc] + acc[y];
	if (CheckDest(instr.dest, z)) acc[z] = vf[instr.fs][z] * vf[instr.ft][bc] + acc[z];
	if (CheckDest(instr.dest, w)) acc[w] = vf[instr.fs][w] * vf[instr.ft][bc] + acc[w];
}

void VU::mulabc(Instruction instr) {
	const auto bc = instr.raw & 3;

	if (CheckDest(instr.dest, x)) acc[x] = vf[instr.fs][x] * vf[instr.ft][bc];
	if (CheckDest(instr.dest, y)) acc[y] = vf[instr.fs][y] * vf[instr.ft][bc];
	if (CheckDest(instr.dest, z)) acc[z] = vf[instr.fs][z] * vf[instr.ft][bc];
	if (CheckDest(instr.dest, w)) acc[w] = vf[instr.fs][w] * vf[instr.ft][bc];
}

void VU::sqi(Instruction instr) {
	const auto addr = vi[instr.it] * 16;

	if (CheckDest(instr.dest, x)) Write32(addr + 0x0, vf[instr.fs][x]);
	if (CheckDest(instr.dest, y)) Write32(addr + 0x4, vf[instr.fs][y]);
	if (CheckDest(instr.dest, z)) Write32(addr + 0x8, vf[instr.fs][z]);
	if (CheckDest(instr.dest, w)) Write32(addr + 0xC, vf[instr.fs][w]);

	vi[instr.it]++;
}

void VU::div(Instruction instr) {
	q = vf[instr.fs][instr.fsf] / vf[instr.ft][instr.ftf];
}

void VU::iswr(Instruction instr) {
	const auto addr = vi[instr.is] * 16;

	if (CheckDest(instr.dest, x)) Write32(addr + 0x0, vi[instr.it]);
	if (CheckDest(instr.dest, y)) Write32(addr + 0x4, vi[instr.it]);
	if (CheckDest(instr.dest, z)) Write32(addr + 0x8, vi[instr.it]);
	if (CheckDest(instr.dest, w)) Write32(addr + 0xC, vi[instr.it]);
}