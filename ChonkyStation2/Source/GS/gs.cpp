#include "gs.h"

void GS::WriteInternalRegister(int reg, u64 data) {
	Helpers::Debug(Helpers::Log::GSd, "Write 0x%llx to internal register %s\n", data, internal_registers[reg].c_str());
	switch (reg) {
	case 0x51: {
		trxpos.raw = data;
		Helpers::Debug(Helpers::Log::GSd, "Source x:      %d\n", trxpos.src_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Source y:      %d\n", trxpos.src_y.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination x: %d\n", trxpos.dst_x.Value());
		Helpers::Debug(Helpers::Log::GSd, "Destination y: %d\n", trxpos.dst_y.Value());
		break;
	}
	}
}