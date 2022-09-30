#pragma warning(disable : 4996) // Stupid fread warning
#include "memory.h"

// Memory reading/writing
template<>
u8 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u8 data;
		memcpy(&data, &ram[paddr], 1 * sizeof(u8));
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
u16 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u16 data;
		memcpy(&data, &ram[paddr], 1 * sizeof(u16));
		return data;
	}
	if (paddr >= 0x1fc00000 && paddr <= 0x20000000) {
		u16 data;
		memcpy(&data, &bios[paddr - 0x1fc00000], 4 * sizeof(u16));
		return data;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		u16 data;
		memcpy(&data, &scratchpad[vaddr - 0x70000000], 4 * sizeof(u16));
		return data;
	}
	Helpers::Panic("Unhandled read16 from 0x%08x\n", vaddr);
}
template<>
u32 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u32 data;
		memcpy(&data, &ram[paddr], 4 * sizeof(u8));
		return data;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		u32 data;
		memcpy(&data, &scratchpad[vaddr - 0x70000000], 4 * sizeof(u8));
		return data;
	}
	if (paddr >= 0x1fc00000 && paddr <= 0x20000000) {
		u32 data;
		memcpy(&data, &bios[paddr - 0x1fc00000], 4 * sizeof(u8));
		return data;
	}

	// GIF
	if (paddr == 0x10003020) {
		Helpers::Debug(Helpers::Log::GIFd, "Read STAT\n");
		return gif->stat;
	}

	// GS
	if (paddr == 0x12001000) {
		//Helpers::Debug(Helpers::Log::GSd, "Read CSR\n");
		u32 data = gs->csr;
		gs->csr &= ~(1 << 3);	// TODO: Is this when I should clear VSINT?
		return data;
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
	if (paddr == 0x1000e020) {
		Helpers::Debug(Helpers::Log::DMAd, "Read PCR\n");
		return dma->PCR;
	}

	if (paddr == 0x1000f000) {
		return intc_stat;
	}
	if (paddr == 0x1000f010) {
		return intc_mask;
	}

	if (paddr == 0x1000f430) {
		return 0;
	}

	// From ps2tek - EE RDRAM initialization
	if (paddr == 0x1000f440) {
		u8 SOP = (mch_ricm >> 6) & 0xF;
		u8 SA = (mch_ricm >> 16) & 0xFFF;
		if (!SOP) {
			switch (SA) {
			case 0x21:
				if (rdram_sdevid < 2) {
					rdram_sdevid++;
					return 0x1F;
				}
				return 0;
			case 0x23:
				return 0x0D0D;
			case 0x24:
				return 0x0090;
			case 0x40:
				return mch_ricm & 0x1F;
			}
		}
		return 0;
	}

	if (paddr == 0x1c0003c0) return 0; // TODO: Don't know what this is
	if (paddr == 0x1f80141c) return 0; // TODO: Don't know what this is
	if (paddr == 0x1000f130) return 0; // TODO: Don't know what this is
	if (paddr == 0x1000f400) return 0; // TODO: Don't know what this is
	if (paddr == 0x1000f410) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f88) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f90) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003f98) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003fa0) return 0; // TODO: Don't know what this is
	if (paddr == 0x10003fa8) return 0; // TODO: Don't know what this is

	Helpers::Panic("Unhandled read32 from 0x%08x (vaddr: 0x%08x)\n", paddr, vaddr);
}
template<>
u64 Memory::Read(u32 vaddr) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		u64 data;
		memcpy(&data, &ram[paddr], 8 * sizeof(u8));
		return data;
	}
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "Read CSR\n");
		u64 data = gs->csr;
		gs->csr &= ~(1 << 3);	// TODO: Is this when I should clear VSINT?
		return data;
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
		ram[paddr] = data;
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
void Memory::Write(u32 vaddr, u16 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		memcpy(&ram[paddr], &data, sizeof(u16));
		return;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		memcpy(&scratchpad[vaddr - 0x70000000], &data, sizeof(u16));
		return;
	}

	if (paddr == 0x1f801470) return;	// TODO: Don't know what this is
	if (paddr == 0x1f801472) return;	// TODO: Don't know what this is
	if (paddr == 0x1a000008) return;	// TODO: Don't know what this is

	Helpers::Panic("Unhandled write16 to 0x%08x\n", paddr);
}
template<>
void Memory::Write(u32 vaddr, u32 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		memcpy(&ram[paddr], &data, sizeof(u32));
		return;
	}
	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		memcpy(&scratchpad[vaddr - 0x70000000], &data, sizeof(u32));
		return;
	}

	// TIMERS (stubbed)
	if (paddr >= 0x10000000 && (paddr <= (0x10000030 + 0x800 * 4))) {
		printf("Write to TIMER registers\n");
		return;
	}

	// GS
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "CSR <- 0x%08x\n", data);
		data &= ~(1 << 3);
		gs->csr = data;
		return;
	}

	// GIF
	if (paddr == 0x10003000) {
		Helpers::Debug(Helpers::Log::GIFd, "CTRL <- 0x%08x\n", data);
		gif->ctrl = data;
		return;
	}

	// DMA
	if (paddr == 0x10008000) {
		dma->VIF0.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) CHCR <- 0x%08x\n", data);
		if (dma->VIF0.MaybeStart()) {
			Helpers::Panic("Attempted to start VIF0 DMA\n");
		}
		return;
	}
	if (paddr == 0x10008010) {
		dma->VIF0.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10008030) {
		dma->VIF0.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10008020) {
		dma->VIF0.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10008040) {
		dma->VIF0.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10008050) {
		dma->VIF0.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10008080) {
		dma->VIF0.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF0) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x10009000) {
		dma->VIF1.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) CHCR <- 0x%08x\n", data);
		if (dma->VIF1.MaybeStart()) {
			Helpers::Panic("Attempted to start VIF1 DMA\n");
		}
		return;
	}
	if (paddr == 0x10009010) {
		dma->VIF1.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10009030) {
		dma->VIF1.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10009020) {
		dma->VIF1.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10009040) {
		dma->VIF1.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10009050) {
		dma->VIF1.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x10009080) {
		dma->VIF1.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(VIF1) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000b000) {
		dma->IPU_FROM.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) CHCR <- 0x%08x\n", data);
		if (dma->IPU_FROM.MaybeStart()) {
			Helpers::Panic("Attempted to start IPU_FROM DMA\n");
		}
		return;
	}
	if (paddr == 0x1000b010) {
		dma->IPU_FROM.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b030) {
		dma->IPU_FROM.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b020) {
		dma->IPU_FROM.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b040) {
		dma->IPU_FROM.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b050) {
		dma->IPU_FROM.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b080) {
		dma->IPU_FROM.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_FROM) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000b400) {
		dma->IPU_TO.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) CHCR <- 0x%08x\n", data);
		if (dma->IPU_TO.MaybeStart()) {
			Helpers::Panic("Attempted to start IPU_TO DMA\n");
		}
		return;
	}
	if (paddr == 0x1000b410) {
		dma->IPU_TO.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b430) {
		dma->IPU_TO.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b420) {
		dma->IPU_TO.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b440) {
		dma->IPU_TO.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b450) {
		dma->IPU_TO.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000b480) {
		dma->IPU_TO.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(IPU_TO) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000c000) {
		dma->SIF0.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) CHCR <- 0x%08x\n", data);
		if (dma->SIF0.MaybeStart()) {
			Helpers::Panic("Attempted to start SIF0 DMA\n");
		}
		return;
	}
	if (paddr == 0x1000c010) {
		dma->SIF0.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c030) {
		dma->SIF0.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c020) {
		dma->SIF0.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c040) {
		dma->SIF0.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c050) {
		dma->SIF0.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c080) {
		dma->SIF0.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF0) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000c400) {
		dma->SIF1.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) CHCR <- 0x%08x\n", data);
		if (dma->SIF1.MaybeStart()) {
			Helpers::Panic("Attempted to start SIF1 DMA\n");
		}
		return;
	}
	if (paddr == 0x1000c410) {
		dma->SIF1.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c430) {
		dma->SIF1.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c420) {
		dma->SIF1.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c440) {
		dma->SIF1.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c450) {
		dma->SIF1.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c480) {
		dma->SIF1.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF1) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000c800) {
		dma->SIF2.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) CHCR <- 0x%08x\n", data);
		if (dma->SIF2.MaybeStart()) {
			Helpers::Panic("Attempted to start SIF2 DMA\n");
		}
		return;
	}
	if (paddr == 0x1000c810) {
		dma->SIF2.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c830) {
		dma->SIF2.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c820) {
		dma->SIF2.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c840) {
		dma->SIF2.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c850) {
		dma->SIF2.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000c880) {
		dma->SIF2.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SIF2) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000d000) {
		dma->SPR_FROM.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) CHCR <- 0x%08x\n", data);
		if (dma->SPR_FROM.MaybeStart()) {
			Helpers::Panic("Attempted to start SPR_FROM DMA\n");
		}
		return;
	}
	if (paddr == 0x1000d010) {
		dma->SPR_FROM.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d030) {
		dma->SPR_FROM.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d020) {
		dma->SPR_FROM.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d040) {
		dma->SPR_FROM.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d050) {
		dma->SPR_FROM.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d080) {
		dma->SPR_FROM.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_FROM) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000d400) {
		dma->SPR_TO.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) CHCR <- 0x%08x\n", data);
		if (dma->SPR_TO.MaybeStart()) {
			Helpers::Panic("Attempted to start SPR_TO DMA\n");
		}
		return;
	}
	if (paddr == 0x1000d410) {
		dma->SPR_TO.MADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) MADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d430) {
		dma->SPR_TO.TADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) TADR <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d420) {
		dma->SPR_TO.QWC = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) QWC <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d440) {
		dma->SPR_TO.ASR0 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) ASR0 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d450) {
		dma->SPR_TO.ASR1 = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) ASR1 <- 0x%08x\n", data);
		return;
	}
	if (paddr == 0x1000d480) {
		dma->SPR_TO.SADR = data;
		Helpers::Debug(Helpers::Log::DMAd, "(SPR_TO) SADR <- 0x%08x\n", data);
		return;
	}

	if (paddr == 0x1000a000) {
		dma->GIF.CHCR.raw = data;
		Helpers::Debug(Helpers::Log::DMAd, "(GIF) CHCR <- 0x%08x\n", data);
		if (dma->GIF.MaybeStart()) {
			dma->GIF.DoDMA(ram, &gif->SendQWord, gif);
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

	// VIF0
	if (paddr == 0x10003800) {
		printf("VIF0_STAT write\n");
		return;
	}
	if (paddr == 0x10003810) {
		printf("VIF0_FBRST write\n");
		return;
	}
	if (paddr == 0x10003820) {
		printf("VIF0_ERR write\n");
		return;
	}
	if (paddr == 0x10003830) {
		printf("VIF0_MARK write\n");
		return;
	}

	// VIF1
	if (paddr == 0x10003c00) {
		printf("VIF1_STAT write\n");
		return;
	}
	if (paddr == 0x10003c10) {
		printf("VIF1_FBRST write\n");
		return;
	}

	// INTC
	if (paddr == 0x1000f000) {
		intc_stat &= ~data;
		return;
	}
	if (paddr == 0x1000f010) {
		intc_mask = data;
		return;
	}

	// From ps2tek - EE RDRAM initialization
	if(paddr == 0x1000f430) {
		uint8_t SA = (data >> 16) & 0xFFF;
		uint8_t SBC = (data >> 6) & 0xF;

		if (SA == 0x21 && SBC == 0x1 && ((mch_drd >> 7) & 1) == 0) rdram_sdevid = 0;

		mch_ricm = data & ~0x80000000;
		return;
	}
	if (paddr == 0x1000f440) {
		mch_drd = data;
		return;
	}

	if (paddr == 0x1f80141c) return; // TODO: Don't know what this is
	if (paddr == 0x1000f100) return; // TODO: Don't know what this is
	if (paddr == 0x1000f120) return; // TODO: Don't know what this is
	if (paddr == 0x1000f140) return; // TODO: Don't know what this is
	if (paddr == 0x1000f150) return; // TODO: Don't know what this is
	if (paddr == 0x1000f400) return; // TODO: Don't know what this is
	if (paddr == 0x1000f410) return; // TODO: Don't know what this is
	if (paddr == 0x1000f420) return; // TODO: Don't know what this is
	if (paddr == 0x1000f450) return; // TODO: Don't know what this is
	if (paddr == 0x1000f460) return; // TODO: Don't know what this is
	if (paddr == 0x1000f480) return; // TODO: Don't know what this is
	if (paddr == 0x1000f490) return; // TODO: Don't know what this is
	if (paddr == 0x1000f500) return; // TODO: Don't know what this is
	if (paddr == 0x1000f510) return; // TODO: Don't know what this is

	Helpers::Panic("Unhandled write32 to 0x%08x (vaddr: 0x%08x) @ 0x%08x\n", paddr, vaddr, *pc);
}
template<>
void Memory::Write(u32 vaddr, u64 data) {
	// This is temporary, need to handle all the virtual address stuff
	u32 paddr = vaddr & 0x1FFFFFFF;
	if (paddr >= 0x00000000 && paddr <= 0x00000000 + 32 MB) {
		memcpy(&ram[paddr], &data, sizeof(u64));
		return;
	}

	if (paddr >= 0x11000000 && paddr <= 0x11000000 + 16 KB) {
		memcpy(&vu0_code_mem[paddr - 0x11000000], &data, sizeof(u64));
		return;
	}

	if (paddr >= 0x11004000 && paddr <= 0x11004000 + 16 KB) {
		memcpy(&vu0_data_mem[paddr - 0x11004000], &data, sizeof(u64));
		return;
	}

	if (paddr >= 0x11008000 && paddr <= 0x11008000 + 16 KB) {
		memcpy(&vu1_code_mem[paddr - 0x11008000], &data, sizeof(u64));
		return;
	}

	if (paddr >= 0x1100C000 && paddr <= 0x1100C000 + 16 KB) {
		memcpy(&vu1_data_mem[paddr - 0x1100C000], &data, sizeof(u64));
		return;
	}

	// GIF
	if (paddr >= 0x10006000 && (paddr <= 0x10006000 + 0x10)) {
		Helpers::Debug(Helpers::Log::GIFd, "FIFO <- 0x%08x (unimplemented)\n", data);
		return;
	}

	if (paddr == 0x12000000) {
		Helpers::Debug(Helpers::Log::GSd, "PMODE <- 0x%08x\n", data);
		gs->pmode = data;
		return;
	}
	if (paddr == 0x12000010) {
		Helpers::Debug(Helpers::Log::GSd, "SMODE1 <- 0x%08x\n", data);
		gs->smode1 = data;
		return;
	}
	if (paddr == 0x12000020) {
		Helpers::Debug(Helpers::Log::GSd, "SMODE2 <- 0x%08x\n", data);
		gs->smode2 = data;
		return;
	}
	if (paddr == 0x12000030) {
		Helpers::Debug(Helpers::Log::GSd, "SRFSH <- 0x%08x\n", data);
		gs->synch1 = data;
		return;
	}
	if (paddr == 0x12000040) {
		Helpers::Debug(Helpers::Log::GSd, "SYNCH1 <- 0x%08x\n", data);
		gs->synch1 = data;
		return;
	}
	if (paddr == 0x12000050) {
		Helpers::Debug(Helpers::Log::GSd, "SYNCH2 <- 0x%08x\n", data);
		gs->synch2 = data;
		return;
	}
	if (paddr == 0x12000060) {
		Helpers::Debug(Helpers::Log::GSd, "SYNCV <- 0x%08x\n", data);
		gs->syncv = data;
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
	if (paddr == 0x120000e0) {
		Helpers::Debug(Helpers::Log::GSd, "BGCOLOR <- 0x%08x\n", data);
		gs->bgcolor = data;
		return;
	}
	if (paddr == 0x12001000) {
		Helpers::Debug(Helpers::Log::GSd, "CSR <- 0x%08x\n", data);
		data &= ~(1 << 3);
		gs->csr = data;
		return;
	}
	if (paddr == 0x12001010) {
		Helpers::Debug(Helpers::Log::GSd, "IMR <- 0x%08x\n", data);
		gs->imr = data;
		return;
	}

	if (vaddr >= 0x70000000 && vaddr <= 0x70000000 + 16 KB) {
		memcpy(&scratchpad[vaddr - 0x70000000], &data, 8 * sizeof(u8));
		return;
	}

	if (paddr == 0x10004000) return; // TODO: Don't know what this is
	if (paddr == 0x10004008) return; // TODO: Don't know what this is
	if (paddr == 0x10005000) return; // TODO: Don't know what this is
	if (paddr == 0x10005008) return; // TODO: Don't know what this is

	std::ofstream out("ram.bin", std::iostream::binary);
	out.write((const char*)ram, 32 MB);
	Helpers::Panic("Unhandled write64 to 0x%08x (0x%08x) @ 0x%08x\n", paddr, vaddr, *pc);
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