#include "sif.h"

u32 SIF::ReadSIF1(u128 unused, void* sifptr) {
	SIF* sif = (SIF*)sifptr;
	//if (sif->sif1_index >= sif->sif1_fifo.size()) Helpers::Panic("SIF1 FIFO Out of bounds access (index: %d)\n", sif->sif1_index);
	u32 word = sif->sif1_fifo.front();
	sif->sif1_fifo.pop();
	if (sif->sif1_fifo.empty()) sif->sif1_empty = true;
	return word;
}

u32 SIF::SendSIF1(u128 qword, void* sifptr) {
	SIF* sif = (SIF*)sifptr;
	sif->sif1_empty = false;

	//Helpers::Debug(Helpers::Log::SIFd, "FIFO: 0x%08x\n", qword.b32[1]);
	//Helpers::Debug(Helpers::Log::SIFd, "FIFO: 0x%08x\n", qword.b32[0]);
	//Helpers::Debug(Helpers::Log::SIFd, "FIFO: 0x%08x\n", qword.b32[3]);
	//Helpers::Debug(Helpers::Log::SIFd, "FIFO: 0x%08x\n", qword.b32[2]);

	sif->sif1_fifo.push(qword.b32[0]);
	sif->sif1_fifo.push(qword.b32[1]);
	sif->sif1_fifo.push(qword.b32[2]);
	sif->sif1_fifo.push(qword.b32[3]);

	return 0;
}

u32 SIF::ReadSIF0(u128 unused, void* sifptr) {
	SIF* sif = (SIF*)sifptr;
	if (sif->sif0_empty) Helpers::Panic("Tried to read empty SIF0");
	//if (sif->sif1_index >= sif->sif1_fifo.size()) Helpers::Panic("SIF0 FIFO Out of bounds access (index: %d)\n", sif->sif1_index);
	u32 word = sif->sif0_fifo.front();
	sif->sif0_fifo.pop();
	if (sif->sif0_fifo.empty()) sif->sif0_empty = true;
	return word;
}


u32 SIF::SendSIF0(u128 word, void* sifptr) {
	SIF* sif = (SIF*)sifptr;
	sif->sif0_empty = false;

	//Helpers::Debug(Helpers::Log::SIFd, "FIFO: 0x%08x\n", word.b32[0]);

	sif->sif0_fifo.push(word.b32[0]);
	
	return 0;
}