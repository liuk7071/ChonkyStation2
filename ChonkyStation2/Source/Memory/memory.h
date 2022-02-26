#pragma once
#include "../common.h"
#include "../DMA/dma.h"
#include "../GS/gs.h"

class Memory {
public:
	DMA* dma;
	GS* gs;
	Memory(DMA* dmaptr, GS* gsptr) : dma(dmaptr), gs(gsptr) {};
	// Memory regions
	u8* ram = new u8[32 MB];
	
	// Memory reading/writing
	template<typename T> T Read(u32 vaddr);
	template<typename T> void Write(u32 vaddr, T data);

	// ELF
	// Loads an ELF file into memory and returns entry point
	u32 LoadELF(const char* FilePath);
};