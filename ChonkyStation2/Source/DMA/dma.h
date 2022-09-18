#pragma once
#include "../common.h"

class DMAGeneric {
public:
	union chcr {
		u32 raw;
		BitField<0, 1, u32> DIR;
		BitField<2, 2, u32> MOD;
		BitField<4, 2, u32> ASP;
		BitField<6, 1, u32> TTE;
		BitField<7, 1, u32> TIE;
		BitField<8, 1, u32> STR;
		BitField<16, 15, u32> TAG;
	};
	chcr CHCR;
	u32 MADR;
	u32 TADR;
	u32 QWC;
	u32 ASR0;
	u32 ASR1;
	u32 SADR;

	bool MaybeStart();
	void DoDMA(u8* source, void (*dma_handler_ptr)(u128, void*), void* device);
	
	enum DMAModes {
		Normal,
		Chain,
		Interleave
	};

	enum DMADirections {
		ToMem,
		FromMem
	};
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