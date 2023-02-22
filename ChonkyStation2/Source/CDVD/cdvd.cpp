#include "cdvd.h"

void CDVD::SendSCommand(u8 cmd) {
	s_command = cmd;
	Helpers::Debug(Helpers::Log::CDVDd, "S Command: 0x%02x\n", cmd);

	switch (cmd) {
	case S_COMMANDS::UpdateStickyFlags: {
		s_command_status.data_available = 0;
		s_response_length = 1;
		break;
	}
	case S_COMMANDS::OpenConfig: {
		s_command_status.data_available = 0;
		s_response_length = 1;
		break;
	}
	case S_COMMANDS::ReadConfig: {
		s_command_status.data_available = 0;
		s_response_length = 4;
		break;
	}
	default:
		Helpers::Panic("Unhandled CDVD S Command");
	}
}

u32 CDVD::ReadSCommandResponse() {
	s_response_length--;
	if (s_response_length == 0) s_command_status.data_available = 1;
	return 0;
}

void CDVD::SendNParameter(u8 param) {
	Helpers::Debug(Helpers::Log::CDVDd, "N Param: 0x%02x\n", param);
	n_params.push(param);
	return;
}

void CDVD::SendNCommand(u8 cmd) {
	n_command = cmd;
	Helpers::Debug(Helpers::Log::CDVDd, "N Command: 0x%02x\n", cmd);

	switch (cmd) {
	case N_COMMANDS::ReadCD: {
		u32 loc = ReadParam();
		loc |= ReadParam() << 8;
		loc |= ReadParam() << 16;
		loc |= ReadParam() << 24;
		u32 n = ReadParam();
		n |= ReadParam() << 8;
		n |= ReadParam() << 16;
		n |= ReadParam() << 24;
		// Unused
		ReadParam();
		ReadParam();
		u8 block_size_byte = ReadParam();
		int block_size;
		switch (block_size_byte) {
		case 1:  block_size = 2328;
		case 2:  block_size = 2340;
		default: block_size = 2048;
		}
		CDReadSector(loc, block_size, n);
		break;
	}
	default:
		Helpers::Panic("Unhandled CDVD N Command");
	}
}

void CDVD::CDReadSector(u32 loc, int block_size, u32 n) {
	sector_buffer.clear();
	sector_buffer_index = 0;

	Helpers::Debug(Helpers::Log::CDVDd, "Reading sector %d...\n", loc);
	Helpers::Debug(Helpers::Log::CDVDd, "Block size: %d\n", block_size);

	loc *= block_size;
	u8* temp_sector_buffer = new u8[2340];
	fseek(dvd, loc, SEEK_SET);
	for (int sector = 0; sector < n; sector++) {
		fread(temp_sector_buffer, sizeof(u8), block_size, dvd);
		for (int i = 0; i < block_size; i++) sector_buffer.push_back(temp_sector_buffer[i]);
	}
	
	Helpers::Debug(Helpers::Log::CDVDd, "Done.\n", loc);
	irq = true;
	i_stat |= 1;
}

u32 CDVD::ReadSectorBuffer(u128 unused, void* cdvdptr) {
	CDVD* cdvd = (CDVD*)cdvdptr;

	if (cdvd->sector_buffer_index >= cdvd->sector_buffer.size()) Helpers::Panic("CDVD sector buffer overflow!\n");

	u32 word;
	std::memcpy(&word, &cdvd->sector_buffer[cdvd->sector_buffer_index], sizeof(u32));
	cdvd->sector_buffer_index += 4;

	if (cdvd->sector_buffer_index >= (cdvd->sector_buffer.size() - 1)) {	// At this point we read all the data.
		cdvd->irq = false;													// If the function is called again we will overflow; that's checked
		//printf("BOOOOOOP\n");												// by the if statement at the top

	}
	return word;
}