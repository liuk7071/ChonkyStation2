#pragma once
#include "../common.h"
#include <map>

class GS {
public:
	std::map<int, std::string> internal_registers {
		{0x00,		"PRIM"},
		{0x01,		"RGBAQ"},
		{0x02,		"ST"},
		{0x03,		"UV"},
		{0x04,		"XYZF2"},
		{0x05,		"XYZ2"},
		{0x06,		"TEX0_1"},
		{0x07,		"TEX0_2"},
		{0x08,		"CLAMP_1"},
		{0x09,		"CLAMP_2"},
		{0x0A,		"FOG"},
		{0x0C,		"XYZF3"},
		{0x0D,		"XYZ3"},
		{0x14,		"TEX1_1"},
		{0x15,		"TEX1_2"},
		{0x16,		"TEX2_1"},
		{0x17,		"TEX2_2"},
		{0x18,		"XYOFFSET_1"},
		{0x19,		"XYOFFSET_2"},
		{0x1A,		"PRMODECONT"},
		{0x1B,		"PRMODE"},
		{0x1C,		"TEXCLUT"},
		{0x22,		"SCANMSK"},
		{0x34,		"MIPTBP1_1"},
		{0x35,		"MIPTBP1_2"},
		{0x36,		"MIPTBP2_1"},
		{0x37,		"MIPTBP2_2"},
		{0x3B,		"TEXA"},
		{0x3D,		"FOGCOL"},
		{0x3F,		"TEXFLUSH"},
		{0x40,		"SCISSOR_1"},
		{0x41,		"SCISSOR_2"},
		{0x42,		"ALPHA_1"},
		{0x43,		"ALPHA_2"},
		{0x44,		"DIMX"},
		{0x45,		"DTHE"},
		{0x46,		"COLCLAMP"},
		{0x47,		"TEST_1"},
		{0x48,		"TEST_2"},
		{0x49,		"PABE"},
		{0x4A,		"FBA_1"},
		{0x4B,		"FBA_2"},
		{0x4C,		"FRAME_1"},
		{0x4D,		"FRAME_2"},
		{0x4E,		"ZBUF_1"},
		{0x4F,		"ZBUF_2"},
		{0x50,		"BITBLTBUF"},
		{0x51,		"TRXPOS"},
		{0x52,		"TRXREG"},
		{0x53,		"TRXDIR"},
		{0x54,		"HWREG"},
		{0x60,		"SIGNAL"},
		{0x61,		"FINISH"},
		{0x62,		"LABEL"}
	};
	void WriteInternalRegister(int reg, u64 data);

	u64 pmode;
	u64 smode2;
	u64 dispfb2;
	u64 display2;
	u64 csr;
	u64 imr;
	union {
		u64 raw;
		BitField<0, 11, u64> src_x;
		BitField<16, 11, u64> src_y;
		BitField<32, 11, u64> dst_x;
		BitField<48, 11, u64> dst_y;
		BitField<59, 2, u64> transmission_order;
	} trxpos;
};