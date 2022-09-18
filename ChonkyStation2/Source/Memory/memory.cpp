#pragma warning(disable : 4996) // Stupid fread warning
#include "memory.h"

// Memory reading/writing
template<>
u8 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u8 data;
		memcpy(&data, &ram[vaddr], 1 * sizeof(u8));
		return data;
	}
	if (paddr >= 0x1fc00000 && paddr <= 0x20000000) {
		u8 data;
		memcpy(&data, &bios[paddr - 0x1fc00000], 4 * sizeof(u8));
		return data;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		return scratchpad[vaddr - 0x70000000];
	}

	if (paddr == 0x1fc7ff52) {
		return 2; // 2=NTSC, 3=PAL
	}
	Helpers::Panic("Unhandled read8 from 0x%08x\n", vaddr);
}
template<>
u32 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u32 data;
		memcpy(&data, &ram[vaddr], 4 * sizeof(u8));
		return data;
	}
	if (paddr >= 0x1fc00000 && paddr <= 0x20000000) {
		u32 data;
		memcpy(&data, &bios[paddr - 0x1fc00000], 4 * sizeof(u8));
		return data;
	}

	// GS
	if (paddr == 0x12001000) {
		//Helpers::Debug(Helpers::Log::GSd, "Read CSR\n");
		return gs->csr | (1 << 3); // Pretend VSYNC interrupt happened
	}

	// DMA
	if(paddr == 0x1000a000) {
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) Read CHCR\n");
		return dma->GIF.CHCR.raw;
	}
	if (paddr == 0x1000e000) {
		Helpers::Debug(Helpers::Log::DMAd, "Read CTRL\n");
		return dma->CTRL;
	}
	if (paddr == 0x1000e010) {
		Helpers::Debug(Helpers::Log::DMAd, "Read STAT\n");
		return dma->STAT;
	}

	if (paddr == 0x1000f130) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f88) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f90) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f98) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003fa0) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003fa8) return 0; // TODO: Don't know what this is

	Helpers::Panic("Unhandled read32 from 0x%08x\n", paddr);
}
template<>
u64 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u64 data;
		memcpy(&data, &ram[vaddr], 8 * sizeof(u8));
		return data;
	}
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "Read CSR\n");
		return gs->csr;
	}

	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		u64 data;
		memcpy(&data, &scratchpad[vaddr - 0x70000000], 8 * sizeof(u8));
		return data;
	}

	Helpers::Panic("Unhandled read64 from 0x%08x\n", vaddr);
}

template<>
void Memory::Write(u32 vaddr, u8 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		ram[vaddr] = data;
		return;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		scratchpad[vaddr - 0x70000000] = data;
		return;
	}

	if (paddr == 0x1000f180) {
		printf("%c", data);
		return;
	}
	Helpers::Panic("Unhandled write8 to 0x%08x\n", vaddr);
}
template<>
void Memory::Write(u32 vaddr, u32 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		memcpy(&ram[paddr], &data, 4 * sizeof(u8));
		return;
	}

	// GS
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "CSR <- 0x%08x\n", data);
		gs->csr = data;
		return;
	}

	// DMA
	if (paddr == 0x1000a000) {
		dma->GIF.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) CHCR <- 0x%08x\n", data);
		if (dma->GIF.MaybeStart()) {
			dma->GIF.DoDMA(&ram[dma->GIF.MADR], &gif->SendQWord, gif);
		}
		return;
	}
	if (paddr == 0x1000a010) {
		dma->GIF.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000a030) {
		dma->GIF.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000a020) {
		dma->GIF.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000a040) {
		dma->GIF.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000a050) {
		dma->GIF.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000a080) {
		dma->GIF.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) SADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e000) {
		dma->CTRL = data;
		Helpers::Debug(Helpers::Log::DMAd, "CTRL <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e010) {
		u16 stat, mask;
		stat = dma->STAT & 0xffff;
		mask = (dma->STAT >> 16) & 0xffff;
		stat &= ~(data & 0xffff);
		mask ^= (data >> 16) & 0xffff;
		dma->STAT = stat | (mask << 16);
		Helpers::Debug(Helpers::Log::DMAd, "STAT <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e020) {
		dma->PCR = data;
		Helpers::Debug(Helpers::Log::DMAd, "PCR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e030) {
		dma->SQWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "SQWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e040) {
		dma->RBSR = data;
		Helpers::Debug(Helpers::Log::DMAd, "RBSR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000e050) {
		dma->RBOR = data;
		Helpers::Debug(Helpers::Log::DMAd, "RBOR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000f100) return; // TODO: Don't know what this is
	if (paddr == 0x1000f120) return; // TODO: Don't know what this is
	if (paddr == 0x1000f140) return; // TODO: Don't know what this is
	if (paddr == 0x1000f150) return; // TODO: Don't know what this is
	if (paddr == 0x1000f500) return; // TODO: Don't know what this is

	Helpers::Panic("Unhandled write32 to 0x%08x\n", paddr);
}
template<>
void Memory::Write(u32 vaddr, u64 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr == 0x12000000) {
		Helpers::Debug(Helpers::Log::GSd, "PMODE <- 0x%08x\n", data);
		gs->pmode = data;
		return;
	}
	if (paddr == 0x12000020) {
		Helpers::Debug(Helpers::Log::GSd, "SMODE2 <- 0x%08x\n", data);
		gs->smode2 = data;
		return;
	}
	if (paddr == 0x12000090) {
		Helpers::Debug(Helpers::Log::GSd, "DISPFB2 <- 0x%08x\n", data);
		gs->dispfb2 = data;
		return;
	}
	if (paddr == 0x120000a0) {
		Helpers::Debug(Helpers::Log::GSd, "DISPLAY2 <- 0x%08x\n", data);
		gs->display2 = data;
		return;
	}
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "CSR <- 0x%08x\n", data);
		gs->csr = data;
		return;
	}

	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		memcpy(&scratchpad[vaddr - 0x70000000], &data, 8 * sizeof(u8));
		return;
	}
	
	Helpers::Panic("Unhandled write64 to 0x%08x\n", paddr);
}

void Memory::LoadBIOS(const char* FilePath) {
	FILE* biosfile = fopen(FilePath, "rb");
	fseek(biosfile, 0, SEEK_SET);
	fread(bios, sizeof(u8), 4 MB, biosfile);
}

// ELF
// Loads an ELF file into memory and returns the entry point
u32 Memory::LoadELF(const char* FilePath) {
	FILE* elf = fopen(FilePath, "rb");
	// Read entry point
	fseek(elf, 0x18, SEEK_SET);
	u32 e_entry = 0;
	fread(&e_entry, sizeof(uint8_t), 4, elf);
	Helpers::Debug(Helpers::Log::ELFd, "Loading ELF -> 0x%08x\n", e_entry);
	// Load the program into RAM
	// TODO: Multiple program headers?
	u32 e_phoff = 0;
	u32 e_shoff = 0;
	u32 e_phentsize = 0;
	u32 e_phnum = 0;
	u32 e_shentsize = 0;
	u32 e_shnum = 0;
	fseek(elf, 0x1c, SEEK_SET);
	fread(&e_phoff, sizeof(uint8_t), 4, elf);
	fread(&e_shoff, sizeof(uint8_t), 4, elf);
	fseek(elf, 0x2a, SEEK_SET);
	fread(&e_phentsize, sizeof(uint8_t), 2, elf);
	fread(&e_phnum, sizeof(uint8_t), 2, elf);
	fread(&e_shentsize, sizeof(uint8_t), 2, elf);
	fread(&e_shnum, sizeof(uint8_t), 2, elf);
	Helpers::Debug(Helpers::Log::ELFd, "(e_phoff     = 0x%08x)\n", e_phoff);
	Helpers::Debug(Helpers::Log::ELFd, "(e_shoff     = 0x%08x)\n", e_shoff);
	Helpers::Debug(Helpers::Log::ELFd, "(e_phentsize = 0x%04x)\n", e_phentsize);
	Helpers::Debug(Helpers::Log::ELFd, "(e_phnum     = 0x%04x)\n", e_phnum);
	Helpers::Debug(Helpers::Log::ELFd, "(e_shentsize = 0x%04x)\n", e_shentsize);
	Helpers::Debug(Helpers::Log::ELFd, "(e_shnum     = 0x%04x)\n", e_shnum);
	Helpers::Debug(Helpers::Log::ELFd, "Found %d program header(s)\n", e_phnum);
	u32 p_offset = 0;
	u32 p_vaddr = 0;
	u32 p_filesz = 0;
	u32 p_memsz = 0;
	fseek(elf, e_phoff + 0x04, SEEK_SET);
	fread(&p_offset, sizeof(u8), 4, elf);
	fread(&p_vaddr, sizeof(u8), 4, elf);
	fseek(elf, e_phoff + 0x10, SEEK_SET);
	fread(&p_filesz, sizeof(u8), 4, elf);
	fread(&p_memsz, sizeof(u8), 4, elf);
	Helpers::Debug(Helpers::Log::ELFd, "(p_offset = 0x%08x)\n", p_offset);
	Helpers::Debug(Helpers::Log::ELFd, "(p_vaddr  = 0x%08x)\n", p_vaddr);
	Helpers::Debug(Helpers::Log::ELFd, "(p_filesz = 0x%08x)\n", p_filesz);
	Helpers::Debug(Helpers::Log::ELFd, "(p_memsz  = 0x%08x)\n", p_memsz);
	fseek(elf, p_offset, SEEK_SET);
	fread(ram + p_vaddr, sizeof(u8), p_filesz, elf);

	Helpers::Debug(Helpers::Log::ELFd, "\n\n--- EXECUTING ---\n");
	return e_entry;
}