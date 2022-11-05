#include "gs.h"

GS::GS() {
	vram.create(2048, 2048, GL_RGB8);
	fb.createWithTexture(vram);
	fb.bind(OpenGL::FramebufferTypes::DrawAndReadFramebuffer);
	OpenGL::setViewport(640, 480);

	vbo.create();
	vbo.bind();

	vao.create();
	vao.bind();
	/*
		VAO format (TODO: I will probably add more attributes)
		
		x, y, z, r, g, b, a = uint
		Vtx1	x	y	z   r   g   b   a
		Vtx2	x	y	z   r   g   b   a		stride: 7*sizeof(uint)
		Vtx3	x	y	z   r   g   b   a
	*/
	vao.setAttributeInt<GLuint>(0, 3, 7 * sizeof(GLuint), (void*)0);
	vao.enableAttribute(0);
	vao.setAttributeInt<GLuint>(1, 4, 7 * sizeof(GLuint), (void*)(3 * sizeof(GLuint)));
	vao.enableAttribute(1);

	if(!vertex_shader.create(vertex_shader_source, OpenGL::ShaderType::Vertex)) Helpers::Panic("Failed to compile vertex shader\n");
	if(!fragment_shader.create(fragment_shader_source, OpenGL::ShaderType::Fragment)) Helpers::Panic("Failed to compile fragment shader\n");
	if(!shader_program.create({ vertex_shader, fragment_shader })) Helpers::Panic("Failed to link shader\n");

	shader_program.use();
}

void GS::WriteInternalRegister(int reg, u64 data) {
	Helpers::Debug(Helpers::Log::GSd, "Write 0x%llx to internal register %s\n", data, internal_registers[reg].c_str());
	switch (reg) {
	case 0x00: {	// PRIM
		prim = data;
		break;
	}
	case 0x01: {	// RGBAQ
		rgbaq.raw = data;
		break;
	}
	case 0x05: {	// XYZ2
		Vertex vertex;
		vertex.coords.x() = data & 0xffff;
		vertex.coords.y() = (data >> 16) & 0xffff;
		vertex.coords.z() = (data >> 32) & 0xffffffff;
		//if((prim & 7) == 0) PushXYZ(vertex);
		break;
	}
	case 0x50: {	// BITBLTBUF
		bitbltbuf.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Source format:      %d\n", bitbltbuf.src_format.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination format: %d\n", bitbltbuf.dst_format.Value());
		break;
	}
	case 0x51: {	// TRXPOS
		trxpos.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Source x:      %d\n", trxpos.src_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Source y:      %d\n", trxpos.src_y.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination x: %d\n", trxpos.dst_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination y: %d\n", trxpos.dst_y.Value());
		break;
	}
	case 0x52: {	// TRXREG
		trxreg.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Width:  %d\n", trxreg.width.Value());
		Helpers::Debug(Helpers::Log::GSd, "Height: %d\n", trxreg.height.Value());
		transfer_pixels_left = trxreg.width * trxreg.height;
		break;
	}
	case 0x53: {	// TRXDIR
		trxdir.raw = data;
		if (trxdir.dir == 2) ProcessCopy(); // TODO: Is this when the VRAM->VRAM transfers should happen...?
		Helpers::Debug(Helpers::Log::GSd, "Direction:  %d\n", trxdir.dir.Value());
		break;
	}
	}
}

void GS::PushHWREG(u64 data) {
	transfer_pixels_left -= 2;
	transfer_buffer.push_back(data & 0xffffffff);
	transfer_buffer.push_back(data >> 32);
	if (transfer_pixels_left == 0) ProcessUpload();
}

void GS::PushXYZ(Vertex vertex) {
	vertex.col.r() = rgbaq.r;
	vertex.col.g() = rgbaq.g;
	vertex.col.b() = rgbaq.b;
	vertex.col.a() = rgbaq.a;
	Helpers::Debug(Helpers::Log::GSd, "Queued vertex:\n");
	Helpers::Debug(Helpers::Log::GSd, "x: %d\n", vertex.coords.x());
	Helpers::Debug(Helpers::Log::GSd, "y: %d\n", vertex.coords.y());
	Helpers::Debug(Helpers::Log::GSd, "z: %d\n", vertex.coords.z());
	Helpers::Debug(Helpers::Log::GSd, "r: %d\n", vertex.col.r());
	Helpers::Debug(Helpers::Log::GSd, "g: %d\n", vertex.col.g());
	Helpers::Debug(Helpers::Log::GSd, "b: %d\n", vertex.col.b());
	Helpers::Debug(Helpers::Log::GSd, "a: %d\n", vertex.col.a());
	vertex_queue.push_back(vertex);

	// Drawing kick
	if (vertex_queue.size() == 3) {	// TODO: Currently only works with triangles (3 vertices)
		Helpers::Debug(Helpers::Log::GSd, "(DRAWING KICK)\n");
		unsigned int attribs[] = {
			vertex_queue[0].coords.x(), vertex_queue[0].coords.y(), vertex_queue[0].coords.z(), vertex_queue[0].col.r(), vertex_queue[0].col.g(), vertex_queue[0].col.b(), vertex_queue[0].col.a(),
			vertex_queue[1].coords.x(), vertex_queue[1].coords.y(), vertex_queue[1].coords.z(), vertex_queue[1].col.r(), vertex_queue[1].col.g(), vertex_queue[1].col.b(), vertex_queue[1].col.a(),
			vertex_queue[2].coords.x(), vertex_queue[2].coords.y(), vertex_queue[2].coords.z(), vertex_queue[2].col.r(), vertex_queue[2].col.g(), vertex_queue[2].col.b(), vertex_queue[2].col.a()
		};
		/*unsigned int attribs[] = {
			31680, 32752, 0,
			31664, 31752, 0,
			31680, 31752, 0
		};*/
		//unsigned int attribs[] = {
		//	20000, 10000, 0,
		//	10000, 30000, 0,
		//	30000, 30000, 0
		//};
		glBufferData(GL_ARRAY_BUFFER, sizeof(attribs), attribs, GL_STATIC_DRAW);
		OpenGL::draw(OpenGL::Primitives::Triangle, 3);
		vertex_queue.clear();
	}
}

// Upload data transferred via GIF to vram
void GS::ProcessUpload() {
	Helpers::Debug(Helpers::Log::GSd, "Uploading texture...\n");
	Helpers::Debug(Helpers::Log::GSd, "Destination:\n");
	Helpers::Debug(Helpers::Log::GSd, "     x: %d\n", trxpos.dst_x.Value());
	Helpers::Debug(Helpers::Log::GSd, "     y: %d\n", trxpos.dst_y.Value());
	Helpers::Debug(Helpers::Log::GSd, "Size:\n");
	Helpers::Debug(Helpers::Log::GSd, "     width : %d\n", trxreg.width.Value());
	Helpers::Debug(Helpers::Log::GSd, "     height: %d\n", trxreg.height.Value());
	Helpers::Debug(Helpers::Log::GSd, "     total : %d\n", trxreg.width * trxreg.height);
	vram.bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, trxpos.dst_x, trxpos.dst_y, trxreg.width, trxreg.height, GL_RGBA, GL_UNSIGNED_BYTE, transfer_buffer.data());
	Helpers::Debug(Helpers::Log::GSd, "Done.\n");
	transfer_buffer.clear();
}

// VRAM->VRAM transfers
void GS::ProcessCopy() {
	Helpers::Debug(Helpers::Log::GSd, "Copying texture\n");
	glBlitFramebuffer(trxpos.src_x, trxpos.src_y, trxpos.src_x + trxreg.width, trxpos.src_y + trxreg.height, trxpos.dst_x, trxpos.dst_y, trxpos.dst_x + trxreg.width, trxpos.dst_y + trxreg.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}