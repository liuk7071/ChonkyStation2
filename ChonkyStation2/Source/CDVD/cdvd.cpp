#include "cdvd.h"

void CDVD::Step() {
	if (read_queued) {
		counter -= 2;
		if (counter == 0) {
			read_queued = false;
			if (queued == queued_t::CD)
				CDReadSector();
			else if (queued == queued_t::DVD)
				DVDReadSector();
			else
				Helpers::Panic("invalid value of queued when reading\n");
		}
	}
}

void CDVD::SendSCommand(u8 cmd) {
	s_command = cmd;
	Helpers::Debug(Helpers::Log::CDVDd, "S Command: 0x%02x\n", cmd);

	switch (cmd) {
	case S_COMMANDS::Subcommand: {
		u8 subc = s_params.front();
		s_params.pop();
		switch (subc) {
		case 0: {	// MechaconVersion
			s_command_status.data_available = 0;
			s_response_length = 4;
			break;
		}
		default:
			Helpers::Panic("Unimplemented S Subcommand 0x%02x\n", subc);
		}
		break;
	}
	case S_COMMANDS::UpdateStickyFlags: {
		sticky_status = status;
		s_command_status.data_available = 0;
		s_response_length = 1;
		break;
	}
	case S_COMMANDS::ReadRTC: {
		s_command_status.data_available = 0;
		s_response_length = 8;
		break;
	}
	case S_COMMANDS::OpenConfig: {
		s_command_status.data_available = 0;
		s_response_length = 1;
		break;
	}
	case S_COMMANDS::ReadConfig: {
		s_command_status.data_available = 0;
		s_response_length = 4 * 4;
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

void CDVD::SendSParameter(u8 param) {
	Helpers::Debug(Helpers::Log::CDVDd, "S Param: 0x%02x\n", param);
	s_params.push(param);
	return;
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
		status |= 1 << 2;
		sticky_status |= status;

		loc = ReadParam();
		loc |= ReadParam() << 8;
		loc |= ReadParam() << 16;
		loc |= ReadParam() << 24;
		n = ReadParam();
		n |= ReadParam() << 8;
		n |= ReadParam() << 16;
		n |= ReadParam() << 24;
		// Unused
		ReadParam();
		ReadParam();
		u8 block_size_byte = ReadParam();
		switch (block_size_byte) {
		case 1:  block_size = 2328;
		case 2:  block_size = 2340;
		default: block_size = 2048;
		}

		queued = queued_t::CD;
		QueueRead();
		break;
	}
	case N_COMMANDS::ReadDVD: {
		status |= 1 << 2;
		sticky_status |= status;

		loc = ReadParam();
		loc |= ReadParam() << 8;
		loc |= ReadParam() << 16;
		loc |= ReadParam() << 24;
		n = ReadParam();
		n |= ReadParam() << 8;
		n |= ReadParam() << 16;
		n |= ReadParam() << 24;

		queued = queued_t::DVD;
		QueueRead();
		break;
	}
	default:
		Helpers::Panic("Unhandled CDVD N Command 0x%02x\n", cmd);
	}
	
	while (!n_params.empty()) n_params.pop();	// Clear queue
}

void CDVD::Break() {
	Helpers::Debug(Helpers::Log::CDVDd, "CDVD Break!\n");
	read_queued = false;
}

void CDVD::QueueRead() {
	if (read_queued) Helpers::Panic("Tried to queue a read, but one was already queued\n");
	read_queued = true;
	counter = 5000;
	Helpers::Debug(Helpers::Log::CDVDd, "Queued CD read in %d cycles\n", counter);
}

void CDVD::CDReadSector() {
	status &= ~(1 << 2); // Done reading
	sticky_status |= status;

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
	i_stat |= 1 << 1;
	i_stat |= 1;
}

void CDVD::DVDReadSector() {
	status &= ~(1 << 2); // Done reading
	sticky_status |= status;

	sector_buffer.clear();
	sector_buffer_index = 0;

	Helpers::Debug(Helpers::Log::CDVDd, "Reading %d sectors at %d...\n", n, loc);
	Helpers::Debug(Helpers::Log::CDVDd, "Block size: 2048 (DVD)\n");
	u32 old_loc = loc;
	loc *= 2048;
	u8* temp_sector_buffer = new u8[2048];
	fseek(dvd, loc, SEEK_SET);
	for (int sector = 0; sector < n; sector++) {
		// Metadata
		sector_buffer.push_back(0 + 0x20);	// Volume number + 0x20
		u32 temp = old_loc - 0 + 0x30000;
		sector_buffer.push_back(temp >> 16);	// Sector number - volume start + 0x30000, in big-endian.
		sector_buffer.push_back(temp >> 8);
		sector_buffer.push_back(temp);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		fread(temp_sector_buffer, sizeof(u8), 2048, dvd);
		for (int i = 0; i < 2048; i++) sector_buffer.push_back(temp_sector_buffer[i]);
		old_loc++;
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
		sector_buffer.push_back(0);
	}

	Helpers::Debug(Helpers::Log::CDVDd, "Done.\n", loc);
	irq = true;
	i_stat |= 1 << 1;
	i_stat |= 1;
}

u32 CDVD::ReadSectorBuffer(u128 unused, void* cdvdptr) {
	CDVD* cdvd = (CDVD*)cdvdptr;
	if (cdvd->sector_buffer_index >= cdvd->sector_buffer.size()) Helpers::Panic("CDVD sector buffer overflow!\n");

	u32 word;
	std::memcpy(&word, &cdvd->sector_buffer[cdvd->sector_buffer_index], sizeof(u32));
	cdvd->sector_buffer_index += 4;

	//if (cdvd->queued == queued_t::DVD) printf("0x%08x\n", word);

	if (cdvd->sector_buffer_index >= (cdvd->sector_buffer.size() - 1)) {	// At this point we read all the data.
		cdvd->irq = false;													// If the function is called again we will overflow; that's checked
		//printf("All data read\n");												// by the if statement at the top
	}
	return word;
}