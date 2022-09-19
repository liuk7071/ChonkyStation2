#include "dma.h"

bool DMAGeneric::MaybeStart() {
	return CHCR.STR;
}

void DMAGeneric::DoDMA(u8* source, void (*dma_handler_ptr)(u128, void*), void* device) {
	switch (CHCR.MOD) {
	case DMAModes::Normal: {
		switch (CHCR.DIR) {
		case DMADirections::FromMem: {
			for (int i = 0; i < (QWC * 16); i += 16) {
				u128 qw;
				qw.b64[0] = ((source[i + 0]) << 0) | ((source[i + 1]) << 8) | ((source[i + 2]) << 16) | ((source[i + 3]) << 24) | ((u64)(source[i + 4]) << 32) | ((u64)(source[i + 5]) << 40) | ((u64)(source[i + 6]) << 48) | ((u64)(source[i + 7]) << 56);
				qw.b64[1] = ((source[i + 8]) << 0) | ((source[i + 9]) << 8) | ((source[i + 10]) << 16) | ((source[i + 11]) << 24) | ((u64)(source[i + 12]) << 32) | ((u64)(source[i + 13]) << 40) | ((u64)(source[i + 14]) << 48) | ((u64)(source[i + 15]) << 56);
				//printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);
				(*dma_handler_ptr)(qw, device);
			}
			break;
		}
		default:
			Helpers::Panic("Unhandled DMA Normal mode direction: %d\n", CHCR.DIR.Value());
		}
		break;
	}
	case DMAModes::Chain: {
		switch (CHCR.DIR) {
		case DMADirections::FromMem: {
			for (int i = 0; i < (QWC * 16); i += 16) {
				u128 qw;
				qw.b64[0] = ((source[i + 0]) << 0) | ((source[i + 1]) << 8) | ((source[i + 2]) << 16) | ((source[i + 3]) << 24) | ((u64)(source[i + 4]) << 32) | ((u64)(source[i + 5]) << 40) | ((u64)(source[i + 6]) << 48) | ((u64)(source[i + 7]) << 56);
				qw.b64[1] = ((source[i + 8]) << 0) | ((source[i + 9]) << 8) | ((source[i + 10]) << 16) | ((source[i + 11]) << 24) | ((u64)(source[i + 12]) << 32) | ((u64)(source[i + 13]) << 40) | ((u64)(source[i + 14]) << 48) | ((u64)(source[i + 15]) << 56);
				//printf("[DMAC] Transferring QWORD: 0x%016llx%016llx\n", qw.b64[1], qw.b64[0]);
				(*dma_handler_ptr)(qw, device);
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