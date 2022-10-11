#pragma once
#include "../common.h"
#include "../DMA/dma.h"
#include "../GS/gs.h"
#include "../GIF/gif.h"
#include <fstream>

class Memory {
public:
	DMA* dma;
	GIF* gif;
	GS* gs;
	Memory(DMA* dmaptr, GIF* gifptr, GS* gsptr) : dma(dmaptr), gif(gifptr), gs(gsptr) {};
	u32* pc;
	// Memory regions
	u8* ram = new u8[32 MB];
	u8* vu0_code_mem = new u8[16 KB];
	u8* vu0_data_mem = new u8[16 KB];
	u8* vu1_code_mem = new u8[16 KB];
	u8* vu1_data_mem = new u8[16 KB];
	u8* scratchpad = new u8[16 KB];
	u8* bios = new u8[4 MB];

	u16 intc_stat;
	u16 intc_mask;

	int print_cnt = 0; // Used to know when to load an ELF, probably wont work with all bioses, but I don't care
	bool int1 = false; // Temporary stub for int1

	u32 mch_ricm;
	u32 mch_drd;
	int rdram_sdevid;
	
	// Memory reading/writing
	template<typename T> T Read(u32 vaddr);
	template<typename T> void Write(u32 vaddr, T data);

	void LoadBIOS(const char* FilePath);
	// ELF
	// Loads an ELF file into memory and returns entry point
	u32 LoadELF(const char* FilePath);
};