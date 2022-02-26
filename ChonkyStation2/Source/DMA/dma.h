#pragma once
#include "../common.h"

struct DMAGeneric {
	u32 CHCR; // TODO: Use a bitfield for CHCR?
	u32 MADR;
	u32 TADR;
	u32 QWC;
	u32 ASR0;
	u32 ASR1;
	u32 SADR;
};

class DMA {
public:
	// Channels
	DMAGeneric GIF;
	// Registers
	u32 CTRL;
	u32 STAT;
	u32 PCR;
	u32 SQWC;
	u32 RBSR;
	u32 RBOR;
};