#pragma once
#include "../common.h"
#include "../DMA/dma.h"
#include "../GS/gs.h"
#include "../GIF/gif.h"

class Memory {
public:
	DMA* dma;
	GIF* gif;
	GS* gs;
	Memory(DMA* dmaptr, GIF* gifptr, GS* gsptr) : dma(dmaptr), gif(gifptr), gs(gsptr) {};
	// Memory regions
	u8* ram = new u8[32 MB];
	u8* scratchpad = new u8[16 KB];
	u8* bios = new u8[4 MB];
	
	// Memory reading/writing
	template<typename T> T Read(u32 vaddr);
	template<typename T> void Write(u32 vaddr, T data);

	void LoadBIOS(const char* FilePath);
	// ELF
	// Loads an ELF file into memory and returns entry point
	u32 LoadELF(const char* FilePath);
};