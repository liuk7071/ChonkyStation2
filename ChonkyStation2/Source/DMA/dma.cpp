#include "dma.h"

bool DMACGeneric::MaybeStart() {
	return CHCR.STR;
}

void DMACGeneric::DoDMA(u8* source, u32 (*dma_handler_ptr)(u128, void*), void* device) {
	switch (CHCR.MOD) {
	case DMAModes::Normal: {
		u8* transfer_start = source + MADR;
		switch (CHCR.DIR) {
		case DMADirections::FromMem: {
			for (int i = 0; i < (QWC * 16); i += 16) {
				u128 qw;
				std::memcpy(&qw.b64[0], &transfer_start[i], sizeof(u64));
				std::memcpy(&qw.b64[1], &transfer_start[i + 8], sizeof(u64));
				//printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);
				(*dma_handler_ptr)(qw, device);
				MADR += 16;
			}
			break;
		}
		default:
			Helpers::Panic("Unhandled DMA Normal mode direction: %d\n", CHCR.DIR.Value());
		}
		break;
	}
	case DMAModes::Chain: {
		if (is_sif0) sif0_running = true;
		u8* transfer_start = source;
		switch (CHCR.DIR) {
		case DMADirections::ToMem: {
			u128 unused;
			unused.b64[0] = 0;
			unused.b64[1] = 0;
			while (!tag_end) {
				u128 tag;
				tag.b32[0] = (*dma_handler_ptr)(unused, device);
				tag.b32[1] = (*dma_handler_ptr)(unused, device);
				auto tte = ParseDMATag(tag);
				DestTagID();
				if (tte.first) {
					Helpers::Panic("TTE unimplemented\n");
				}
				transfer_start = source + MADR;
				for (int i = 0; i < (QWC * 16); i += 16) {
					u128 qw;
					qw.b32[0] = (*dma_handler_ptr)(unused, device);
					qw.b32[1] = (*dma_handler_ptr)(unused, device);
					qw.b32[2] = (*dma_handler_ptr)(unused, device);
					qw.b32[3] = (*dma_handler_ptr)(unused, device);
					//printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);

					//std::memcpy(&transfer_start[i + 0], &qw.b32[0], sizeof(u32));
					//std::memcpy(&transfer_start[i + 4], &qw.b32[1], sizeof(u32));
					//std::memcpy(&transfer_start[i + 8], &qw.b32[2], sizeof(u32));
					//std::memcpy(&transfer_start[i + 12], &qw.b32[3], sizeof(u32));

					std::memcpy(&transfer_start[i + 0], &qw.b64[0], sizeof(u64));
					std::memcpy(&transfer_start[i + 8], &qw.b64[1], sizeof(u64));
					MADR += 16;
				}
				if (is_sif0) return;
			}
			break;
		}
		case DMADirections::FromMem: {
			while (!tag_end) {
				u128 tag;
				std::memcpy(&tag.b64[0], &source[TADR], sizeof(u64));
				std::memcpy(&tag.b64[1], &source[TADR + 8], sizeof(u64));
				auto tte = ParseDMATag(tag);
				SourceTagID();
				if (tte.first) {
					Helpers::Panic("TTE unimplemented\n");
				}
				transfer_start = source + MADR;
				for (int i = 0; i < (QWC * 16); i += 16) {
					u128 qw;
					std::memcpy(&qw.b64[0], &transfer_start[i], sizeof(u64));
					std::memcpy(&qw.b64[1], &transfer_start[i + 8], sizeof(u64));
					//printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);
					(*dma_handler_ptr)(qw, device);
					MADR += 16;
				}
			}
			break;
		}
		default:
			Helpers::Panic("Unhandled DMA Chain mode direction: %d\n", CHCR.DIR.Value());
		}
		break;	
	}
	default:
		Helpers::Panic("Unhandled DMA mode: %d\n", CHCR.MOD.Value());
	}
	CHCR.STR = false;
	tag_end = false;
	sif0_running = false;
}

std::pair<bool, u64> DMACGeneric::ParseDMATag(u128 tag) {
	/*
	DMAtag Format

	  0-15    QWC to transfer
	  16-25   Unused
	  26-27   Priority control
			  0=No effect
			  1=Reserved
			  2=Priority control disabled (D_PCR.31 = 0)
			  3=Priority control enabled (D_PCR.31 = 1)
	  28-30   Tag ID
	  31      IRQ
	  32-62   ADDR field (lower 4 bits must be zero)
	  63      Memory selection for ADDR (0=RAM, 1=scratchpad)
	  64-127  Data to transfer (only if Dn_CHCR.TTE==1)

	When both IRQ and Dn_CHCR.TIE are set, the transfer ends after QWC has been transferred.
	When Dn_CHCR.TTE is on, bits 64-127 are transferred BEFORE QWC.
	The effects of the tag ID vary depending on if the channel is in source chain or dest chain mode.
	*/
	auto ret = std::make_pair<bool, u64>(false, 0);
	Helpers::Debug(Helpers::Log::DMAd, "Got DMAtag 0x%016llx%016llx (TADR: 0x%08x)\n", tag.b64[1], tag.b64[0], TADR);
	QWC = tag.b64[0] & 0xffff;
	// TODO: Priority control
	tag_id = (tag.b64[0] >> 28) & 7;
	if (((tag.b64[0] >> 31) & 1) && CHCR.TIE) {
		Helpers::Debug(Helpers::Log::DMAd, "IRQ && TIE, last chain transfer\n");
		tag_end = true;
	}
	if (CHCR.TTE) {
		Helpers::Debug(Helpers::Log::DMAd, "TTE, adding 0x%016x to transfer\n", tag.b64[1]);
		ret.first = true;
		ret.second = tag.b64[1];
	}
	dmatag_addr = (tag.b64[0] >> 32) & 0xfffffff0;
	if ((tag.b64[0] >> 63) & 1) Helpers::Panic("DMAtag unimplemented scratchpad ADDR\n");
	Helpers::Debug(Helpers::Log::DMAd, "QWC: %d\n", QWC);
	Helpers::Debug(Helpers::Log::DMAd, "tag id: %d\n", tag_id);
	Helpers::Debug(Helpers::Log::DMAd, "ADDR: 0x%08x\n", dmatag_addr);

	return ret;
}

void DMACGeneric::SourceTagID() {
	switch (tag_id) {
	case 0: {	// refe
		MADR = dmatag_addr;
		TADR += 16;
		tag_end = true;
		break;
	}
	case 1: {	// cnt
		MADR = TADR + 16;
		TADR = MADR + (QWC * 16);
		break;
	}
	case 2: {	// next
		MADR = TADR + 16;
		TADR = dmatag_addr;
		break;
	}
	case 3: {	// ref
		MADR = dmatag_addr;
		TADR += 16;
		break;
	}
	case 7: {	// end
		MADR = TADR + 16;
		tag_end = true;
		break;
	}
	default:
		Helpers::Panic("Unimplemented source tag id value: %d\n", tag_id);
	}
}

void DMACGeneric::DestTagID() {
	switch (tag_id) {
	case 0: {	// cnt
		MADR = dmatag_addr;
		break;
	}
	case 1: {	// cnts
		MADR = dmatag_addr;
		break;
	}
	case 7: {	// end
		MADR = dmatag_addr;
		tag_end = true;
		break;
	}
	default:
		Helpers::Panic("Unimplemented destination tag id value: %d\n", tag_id);
	}
}

// --------  IOP  --------

bool IOPDMAGeneric::MaybeStart() {
	return CHCR.START;
}

void IOPDMAGeneric::DoDMA(u8* ram, u32 (*dma_handler_ptr)(u128, void*), void* device) {
	auto mode = !is_sif ? CHCR.MOD.Value() : 3;

	switch (mode) {
	case IOPDMAModes::Sliced: {
		switch (CHCR.DIR) {
		case IOPDMADirections::ToMem: {
			u128 unused;
			unused.b64[0] = 0;
			unused.b64[1] = 0;
			u8* transfer_dest = ram + (MADR & 0xffffff);
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Sliced DMA\n");
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Block count: %d\n", BCR.COUNT.Value());
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Block size : %d\n", BCR.SIZE.Value());
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Total      : %d\n", BCR.COUNT * BCR.SIZE);
			for (int block = 0; block < BCR.COUNT; block++) {
				for (int i = 0; i < BCR.SIZE; i++) {
					u32 word = (*dma_handler_ptr)(unused, device);
					std::memcpy(transfer_dest, &word, sizeof(u32));
					transfer_dest += 4;
				}
			}
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Done.\n");
			break;
		}
		default:
			Helpers::Panic("Unimplemented IOP Sliced DMA Direction %d\n", CHCR.DIR.Value());
		}
		break;
	}
	case IOPDMAModes::Chain: {
		switch (CHCR.DIR) {
		case IOPDMADirections::ToMem: {
			if(is_sif) sif_running = true;
			u128 unused;
			unused.b64[0] = 0;
			unused.b64[1] = 0;
chain1:
			u64 dma_tag = (*dma_handler_ptr)(unused, device);
			dma_tag |= (u64)((*dma_handler_ptr)(unused, device)) << 32;
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Got DMATag: 0x%016llx\n", dma_tag);
			u8* transfer_dest = ram + (dma_tag & 0xffffff);
			// TODO: Need to handle CHCR.8
			(*dma_handler_ptr)(unused, device);
			(*dma_handler_ptr)(unused, device);
			
			auto size = (dma_tag >> 32) & 0xffffff;
			for (int i = 0; i < size; i++) {
				u32 word = (*dma_handler_ptr)(unused, device);
				//Helpers::Debug(Helpers::Log::DMAd, "(IOP) Writing 0x%08x\n", word);
				std::memcpy(transfer_dest, &word, sizeof(u32));
				transfer_dest += 4;
			}
			if (!((dma_tag >> 31) & 1)) {
				if (is_sif) return;
				goto chain1;
			}
			break;
		}
		case IOPDMADirections::FromMem: {
chain2:
			u64 dma_tag;
			std::memcpy(&dma_tag, &ram[TADR], sizeof(u64));
			Helpers::Debug(Helpers::Log::DMAd, "(IOP) Got DMATag: 0x%016llx\n", dma_tag);
			
			u8* transfer_src = ram + (dma_tag & 0xffffff);
			auto size = (dma_tag >> 32) & 0xffffff;
			if(is_sif) size = (size + 3) & ~0x03; // Round up to next multiple of 4
			if (CHCR.BIT8) {
				u32 word;
				std::memcpy(&word, &ram[TADR + 8], sizeof(u32));
				u128 qword;
				qword.b32[0] = word;
				(*dma_handler_ptr)(qword, device);

				std::memcpy(&word, &ram[TADR + 12], sizeof(u32));
				qword.b32[0] = word;
				(*dma_handler_ptr)(qword, device);
				TADR += 2 * 4;
			}
			TADR += 2 * 4;

			for (int i = 0; i < size; i++) {
				u32 word;
				std::memcpy(&word, transfer_src, sizeof(u32));
				u128 qword;
				qword.b32[0] = word;
				(*dma_handler_ptr)(qword, device);
				transfer_src += 4;
			}
			if (!((dma_tag >> 31) & 1)) goto chain2;
			break;
		}
		default:
			Helpers::Panic("Unimplemented IOP Chain DMA Direction %d\n", CHCR.DIR.Value());
		}
		break;
	}
	default:
		Helpers::Panic("Unimplemented IOP DMA Mode %d\n", mode);
	}

	CHCR.START = 0;
	sif_running = false;
}