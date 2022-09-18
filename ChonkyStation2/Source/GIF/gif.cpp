#include "gif.h"

void GIF::PackedTransfer(u128 qword) {
	switch (regs[current_reg]) {
	case 0xE: {
		auto reg = qword.b64[1] & 0x7f;
		gs->WriteInternalRegister(reg, qword.b64[0]);
		break;
	}
	default:
		Helpers::Panic("Unimplemented GIF packed transfer register 0x%x\n", regs[current_reg]);
	}
	current_nloop--;
	if (current_nloop == 0) {
		current_nloop = nloop;
		current_reg++;
	}
}

void GIF::ImageTransfer(u128 qword) {
	if (current_nloop == 1) {
		has_tag = false;
		gs->ProcessTransfer();
	}
	gs->PushHWREG(qword.b64[0]);
	gs->PushHWREG(qword.b64[1]);
	current_nloop--;
}

void GIF::SendQWord(u128 qword, void* gifptr) {
	GIF* gif = (GIF*)gifptr;
	if ((gif->data_format == DataFormat::PACKED) && (gif->current_reg == gif->nregs)) gif->has_tag = false;
	if (!gif->has_tag) {
		gif->ParseGIFTag(qword);
		return;
	}
	else {
		switch (gif->data_format) {
		case DataFormat::PACKED:
			gif->PackedTransfer(qword);
			break;
		case DataFormat::IMAGE:
			gif->ImageTransfer(qword);
			break;
		default:
			Helpers::Panic("Unimplemented GIF data format %d\n", gif->data_format);
		}
	}
}

void GIF::ParseGIFTag(u128 tag) {
	/*
	GIFtag Format

	  0-14    NLOOP - Data per register to transfer
	  15      EOP - End of packet
	  16-45   Unused
	  46      Enable PRIM field
	  47-57   Data to be sent to GS PRIM register if GIFtag.46 == 1
	  58-59   Data format
			  0=PACKED
			  1=REGLIST
			  2=IMAGE
			  3=IMAGE
	  60-63   NREGS - Number of registers
			  0=16 registers
	  64-127  Register field, 4 bits each
	*/

	has_tag = true;
	Helpers::Debug(Helpers::Log::GIFd, "Got tag\n");
	nloop = tag.b64[0] & 0x7fff;
	current_nloop = nloop;
	data_format = (tag.b64[0] >> 58) & 3;
	nregs = (tag.b64[0] >> 60) & 0xf;
	current_reg = 0;
	if (nregs == 0) nregs = 16;

	Helpers::Debug(Helpers::Log::GIFd, "NLOOP: %d\n", nloop);
	Helpers::Debug(Helpers::Log::GIFd, "Data format: %d\n", data_format);
	Helpers::Debug(Helpers::Log::GIFd, "NREGS: %d\n", nregs);

	Helpers::Debug(Helpers::Log::GIFd, "Reading regs...\n");
	for (int i = 0; i < nregs; i++) {
		regs[i] = (tag.b64[1] >> (i * 4)) & 0xf;
		Helpers::Debug(Helpers::Log::GIFd, "Got reg 0x%x\n", regs[i]);
	}
}