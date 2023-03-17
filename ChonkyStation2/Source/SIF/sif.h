#pragma once
#include "../common.h"
#include <queue>

class SIF {
public:
	u32 mscom = 0;
	u32 smcom = 0;
	u32 msflg = 0;
	u32 smflg = 0;

	bool sif1_empty = true;
	bool iop_sif1_queued = false;
	std::queue<u32> sif1_fifo;
	static u32 ReadSIF1(u128 unused, void* sifptr);
	static u32 SendSIF1(u128 qword, void* sifptr);

	bool sif0_empty = true;
	bool ee_sif0_queued = false;
	bool iop_sif0_stalled = false;
	std::queue<u32> sif0_fifo;
	static u32 ReadSIF0(u128 unused, void* sifptr);
	static u32 SendSIF0(u128 word, void* sifptr);
};