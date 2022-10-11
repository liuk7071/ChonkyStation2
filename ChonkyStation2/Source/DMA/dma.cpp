#include "dma.h"

bool DMAGeneric::MaybeStart() {
	return CHCR.STR;
}

void DMAGeneric::DoDMA(u8* source, void (*dma_handler_ptr)(u128, void*), void* device) {
	switch (CHCR.MOD) {
	case DMAModes::Normal: {
		u8* transfer_start = source + MADR;
		switch (CHCR.DIR) {
		case DMADirections::FromMem: {
			for (int i = 0; i < (QWC * 16); i += 16) {
				u128 qw;
				qw.b64[0] = ((transfer_start[i + 0]) << 0) | ((transfer_start[i + 1]) << 8) | ((transfer_start[i + 2]) << 16) | ((transfer_start[i + 3]) << 24) | ((u64)(transfer_start[i + 4]) << 32) | ((u64)(transfer_start[i + 5]) << 40) | ((u64)(transfer_start[i + 6]) << 48) | ((u64)(transfer_start[i + 7]) << 56);
				qw.b64[1] = ((transfer_start[i + 8]) << 0) | ((transfer_start[i + 9]) << 8) | ((transfer_start[i + 10]) << 16) | ((transfer_start[i + 11]) << 24) | ((u64)(transfer_start[i + 12]) << 32) | ((u64)(transfer_start[i + 13]) << 40) | ((u64)(transfer_start[i + 14]) << 48) | ((u64)(transfer_start[i + 15]) << 56);
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
		u8* transfer_start = source;
		switch (CHCR.DIR) {
		case DMADirections::FromMem: {
			while (!tag_end) {
				u128 tag;
				tag.b64[0] = ((source[TADR + 0]) << 0) | ((source[TADR + 1]) << 8) | ((source[TADR + 2]) << 16) | ((source[TADR + 3]) << 24) | ((u64)(source[TADR + 4]) << 32) | ((u64)(source[TADR + 5]) << 40) | ((u64)(source[TADR + 6]) << 48) | ((u64)(source[TADR + 7]) << 56);
				tag.b64[1] = ((source[TADR + 8]) << 0) | ((source[TADR + 9]) << 8) | ((source[TADR + 10]) << 16) | ((source[TADR + 11]) << 24) | ((u64)(source[TADR + 12]) << 32) | ((u64)(source[TADR + 13]) << 40) | ((u64)(source[TADR + 14]) << 48) | ((u64)(source[TADR + 15]) << 56);
				auto tte = ParseDMATag(tag);
				SourceTagID();
				if (tte.first) {
					Helpers::Panic("TTE unimplemented\n");
				}
				transfer_start = source + MADR;
				for (int i = 0; i < (QWC * 16); i += 16) {
					u128 qw;
					qw.b64[0] = ((transfer_start[i + 0]) << 0) | ((transfer_start[i + 1]) << 8) | ((transfer_start[i + 2]) << 16) | ((transfer_start[i + 3]) << 24) | ((u64)(transfer_start[i + 4]) << 32) | ((u64)(transfer_start[i + 5]) << 40) | ((u64)(transfer_start[i + 6]) << 48) | ((u64)(transfer_start[i + 7]) << 56);
					qw.b64[1] = ((transfer_start[i + 8]) << 0) | ((transfer_start[i + 9]) << 8) | ((transfer_start[i + 10]) << 16) | ((transfer_start[i + 11]) << 24) | ((u64)(transfer_start[i + 12]) << 32) | ((u64)(transfer_start[i + 13]) << 40) | ((u64)(transfer_start[i + 14]) << 48) | ((u64)(transfer_start[i + 15]) << 56);
					printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);
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
}

std::pair<bool, u64> DMAGeneric::ParseDMATag(u128 tag) {
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
	Helpers::Debug(Helpers::Log::DMAd, "Got DMAtag (TADR: 0x%08x)\n", TADR);
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

void DMAGeneric::SourceTagID() {
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
	case 7: {	// end
		MADR = TADR + 16;
		tag_end = true;
		break;
	}
	default:
		Helpers::Panic("Unimplemented source tag id value: %d\n", tag_id);
	}
}