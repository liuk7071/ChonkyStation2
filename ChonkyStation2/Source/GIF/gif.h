#pragma once
#include "../common.h"
#include "../GS/gs.h"

class GIF {
public:
	GS* gs;
	GIF(GS* gsptr) : gs(gsptr) {};

	u128 giftag;
	bool has_tag = false;
	static void SendQWord(u128 qword, void* gifptr);
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
	int current_reg_nloop = 0;

	bool PackedTransfer(u128 qword);
};