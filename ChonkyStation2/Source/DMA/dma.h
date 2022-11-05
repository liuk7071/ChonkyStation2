#pragma once
#include "../common.h"

class DMACGeneric {
public:
	bool is_sif0 = false;
	bool sif0_running = false;

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
	void DoDMA(u8* source, u32 (*dma_handler_ptr)(u128, void*), void* device);
	std::pair<bool, u64> ParseDMATag(u128 tag);
	void SourceTagID();
	void DestTagID();
	int tag_id = 0;
	u32 dmatag_addr = 0;
	bool tag_end = false;
	
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

class DMAC {
public:
	DMAC() {
		SIF0.is_sif0 = true;
	}
	// Channels
	DMACGeneric VIF0;
	DMACGeneric VIF1;
	DMACGeneric GIF;
	DMACGeneric IPU_FROM;
	DMACGeneric IPU_TO;
	DMACGeneric SIF0;
	DMACGeneric SIF1;
	DMACGeneric SIF2;
	DMACGeneric SPR_FROM;
	DMACGeneric SPR_TO;
	// Registers
	u32 CTRL;
	u32 STAT;
	u32 PCR;
	u32 SQWC;
	u32 RBSR;
	u32 RBOR;
};

// --------  IOP  --------

class IOPDMAGeneric {
public:
	bool sif_running = false;
	u32 MADR;
	u32 BCR;
	union chcr {
		u32 raw;
		BitField<0, 1, u32> DIR;
		BitField<1, 1, u32> DECREMENT;
		BitField<8, 1, u32> BIT8;
		BitField<9, 2, u32> MOD;
		BitField<16, 3, u32> DMA_WINDOW_SIZE;
		BitField<20, 3, u32> CPU_WINDOW_SIZE;
		BitField<24, 1, u32> START;
		BitField<28, 1, u32> FORCE_START;
		BitField<29, 1, u32> BIT29;
	};
	chcr CHCR;
	u32 TADR;

	bool is_sif = false; // SIF DMAs only have Chain mode
	bool MaybeStart();
	void DoDMA(u8* ram, u32 (*dma_handler_ptr)(u128, void*), void* device);

	enum IOPDMAModes {
		Burst,
		Slice,
		LinkedList,
		Chain
	};

	enum IOPDMADirections {
		ToMem,
		FromMem
	};
};

class IOPDMA {
public:
	IOPDMA() {
		SIF0.is_sif = true;
		SIF1.is_sif = true;
	}
	// Channels
	IOPDMAGeneric SIF0;
	IOPDMAGeneric SIF1;
	
	u32 DICR;
	union dicr2 {
		u32 raw;
		BitField<0, 13, u32> INTR_ON_TAG;
		BitField<16, 6, u32> IM;
		BitField<24, 6, u32> IF;
	};
	dicr2 DICR2;
};