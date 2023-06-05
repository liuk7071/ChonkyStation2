#pragma once
#include "../common.h"

#define x 0
#define y 1
#define z 2
#define w 3

class VU {
public:
	float vf[32][4];
	float acc[4];
	float q, p;
	u16 vi[16];

	u8* vu0_code_mem;
	u8* vu0_data_mem;

	void ExecVU0Macro(Instruction instr);

	enum SPECIAL1 {
		VMADDx  = 0x08,
		VMADDy  = 0x09,
		VMADDz  = 0x0a,
		VMADDw  = 0x0b,
		VMULq	= 0x1c,
		VIADD   = 0x30,
		VSUB    = 0x2c,
	};
	enum SPECIAL2 {
		VMADDAx = 0x08,
		VMADDAy = 0x09,
		VMADDAz = 0x0a,
		VMADDAw = 0x0b,
		VMULAx  = 0x18,
		VMULAw  = 0x1b,
		VCLIPw  = 0x1f,
		VSQI    = 0x35,
		VDIV    = 0x38,
		VWAITQ  = 0x3b,
		VISWR   = 0x3f
	};

	// Instructions
	bool CheckDest(u8 dest, int component);

	void maddbc(Instruction instr);
	void mulq(Instruction instr);
	void iadd(Instruction instr);
	void sub(Instruction instr);

	void maddabc(Instruction instr);
	void mulabc(Instruction instr);
	void sqi(Instruction instr);
	void div(Instruction instr);
	void iswr(Instruction instr);

	void Write32(u32 addr, u32 data);
};