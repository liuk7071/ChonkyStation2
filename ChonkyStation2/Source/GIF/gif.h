#pragma once
#include "../common.h"
#include "../GS/gs.h"

class GIF {
public:
	using uvec4 = OpenGL::uvec4;
	using vec4 = OpenGL::vec4;

	GS* gs;
	GIF(GS* gsptr) : gs(gsptr) {};

	u128 giftag;
	bool has_tag = false;
	static u32 SendQWord(u128 qword, void* gifptr);
	void ParseGIFTag(u128 tag);

	enum DataFormat {
		PACKED,
		REGLIST,
		IMAGE
	};
	int nloop = 0;
	int nregs = 0;
	int data_format = 0;

	u8 regs[16];
	int current_reg = 0;
	int current_nloop = 0;

	void PackedTransfer(u128 qword);
	void ImageTransfer(u128 qword);

	u32 ctrl = 0;
	u32 stat = 0;
	u32 mode = 0;
};