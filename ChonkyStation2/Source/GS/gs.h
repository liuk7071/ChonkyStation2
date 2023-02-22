#pragma once
#include "../common.h"
#include <map>

class GS {
public:

	GS();
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
	u64 smode1;
	u64 smode2;
	u64 srfsh;
	u64 synch1;
	u64 synch2;
	u64 syncv;
	u64 dispfb1;
	u64 display1;
	u64 dispfb2;
	u64 display2;
	u64 bgcolor;
	u64 csr;
	u64 imr;

	u64 prim;
	union {
		u64 raw;
		BitField<0, 8, u64> r;
		BitField<8, 8, u64> g;
		BitField<16, 8, u64> b;
		BitField<24, 8, u64> a;
		BitField<32, 32, u64> q;
	} rgbaq;
	union {
		u64 raw;
		BitField<0, 14, u64> src_base_ptr;
		BitField<16, 6, u64> src_buff_width;
		BitField<24, 6, u64> src_format;
		BitField<32, 14, u64> dst_base_ptr;
		BitField<48, 6, u64> dst_buff_width;
		BitField<56, 6, u64> dst_format;
	} bitbltbuf;
	union {
		u64 raw;
		BitField<0, 11, u64> src_x;
		BitField<16, 11, u64> src_y;
		BitField<32, 11, u64> dst_x;
		BitField<48, 11, u64> dst_y;
		BitField<59, 2, u64> transmission_order;
	} trxpos;
	union {
		u64 raw;
		BitField<0, 12, u64> width;
		BitField<32, 12, u64> height;
	} trxreg;
	union {
		u64 raw;
		BitField<0, 2, u64> dir;
	} trxdir;
	union {
		u64 raw;
		BitField<0, 16, u64> x;
		BitField<32, 16, u64> y;
	} xyoffset_1;

	OpenGL::Texture vram;
	OpenGL::Framebuffer fb;

	OpenGL::VertexArray vao;
	OpenGL::VertexBuffer vbo;
	OpenGL::Shader vertex_shader;
	OpenGL::Shader fragment_shader;
	OpenGL::Program shader_program;

	const char* vertex_shader_source = R"(
	#version 330 core
	layout(location = 0) in vec3 pos;
	layout(location = 1) in vec4 col;
	uniform vec2 offset;
	
	out vec4 vertex_col;
	
	void main() {
		gl_Position = vec4(((pos.x / 16.f) - (offset.x / 16.f)) / 640.f - 1.f, ((pos.y / 16.f) - (offset.y / 16.f)) / 224.f - 1.f, 0.f, 1.f);
		vertex_col = vec4(float(col.r) / 255.f, float(col.g) / 255.f, float(col.b) / 255.f, 1.f);
	}
	)";

	const char* fragment_shader_source = R"(
	#version 330 core
	in vec4 vertex_col;
	
	out vec4 colour_final;
	
	void main() {
		colour_final = vertex_col;
	}
	)";

	std::vector<u32> transfer_buffer;
	int transfer_pixels_left = 0;
	void PushHWREG(u64 data);
	std::vector<Vertex> vertex_queue;
	int required_vertices = 0;
	void PushXYZ(Vertex vertex);
	void ProcessUpload();
	void ProcessCopy();
};