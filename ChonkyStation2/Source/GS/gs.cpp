#include "gs.h"

GS::GS() {
	vram.create(2048, 2048, GL_RGB8);
	fb.createWithTexture(vram);
	fb.bind(OpenGL::FramebufferTypes::DrawAndReadFramebuffer);
}

void GS::WriteInternalRegister(int reg, u64 data) {
	Helpers::Debug(Helpers::Log::GSd, "Write 0x%llx to internal register %s\n", data, internal_registers[reg].c_str());
	switch (reg) {
	case 0x50: {	// BITBLTBUF
		bitbltbuf.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Source format:      %d\n", bitbltbuf.src_format.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination format: %d\n", bitbltbuf.dst_format.Value());
		break;
	}
	case 0x51: {	// TRXPOS
		trxpos.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Source x:      %d\n", trxpos.src_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Source y:      %d\n", trxpos.src_y.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination x: %d\n", trxpos.dst_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination y: %d\n", trxpos.dst_y.Value());
		break;
	}
	case 0x52: {	// TRXREG
		trxreg.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Width:  %d\n", trxreg.width.Value());
		Helpers::Debug(Helpers::Log::GSd, "Height: %d\n", trxreg.height.Value());
		break;
	}
	case 0x53: {	// TRXDIR
		trxdir.raw = data;
		if (trxdir.dir == 2) ProcessCopy(); // TODO: Is this when the VRAM->VRAM transfers should happen...?
		Helpers::Debug(Helpers::Log::GSd, "Direction:  %d\n", trxdir.dir.Value());
		break;
	}
	}
}

void GS::PushHWREG(u64 data) {
	transfer_buffer.push_back(data & 0xffffffff);
	transfer_buffer.push_back(data >> 32);
}

void GS::PushXYZ(u128 data) {
	// TODO
	Helpers::Debug(Helpers::Log::GSd, "Queued vertex\n");
}

// Upload data transferred via GIF to vram
void GS::ProcessUpload() {
	Helpers::Debug(Helpers::Log::GSd, "Uploading texture\n");
	vram.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, trxpos.dst_x, trxpos.dst_y, trxreg.width, trxreg.height, GL_RGBA, GL_UNSIGNED_BYTE, transfer_buffer.data());
	transfer_buffer.clear();
}

// VRAM->VRAM transfers
void GS::ProcessCopy() {
	Helpers::Debug(Helpers::Log::GSd, "Copying texture\n");
	glBlitFramebuffer(trxpos.src_x, trxpos.src_y, trxpos.src_x + trxreg.width, trxpos.src_y + trxreg.height, trxpos.dst_x, trxpos.dst_y, trxpos.dst_x + trxreg.width, trxpos.dst_y + trxreg.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}