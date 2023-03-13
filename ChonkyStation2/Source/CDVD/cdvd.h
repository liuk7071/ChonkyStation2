#pragma once
#pragma warning(disable : 4996)
#include "../common.h"
#include <queue>

#define IOP_CLOCK (36864000)
#define CD_READ_SPEED (24 * 153600)

class CDVD {
public:
	void Step();

	bool irq = false;
	FILE* dvd;
	u32 i_stat = 0;
	u8 status = 2;
	u8 sticky_status = 0;

	CDVD(const char* dir) {
		dvd = fopen(dir, "rb");
		s_command_status.data_available = 1;
	}

	u8 s_command = 0;
	int s_response_length = 0;
	union S_COMMAND_STATUS {
		u32 raw;
		BitField<6, 1, u32> data_available;
	};
	S_COMMAND_STATUS s_command_status;

	std::queue<u8> s_params;
	void SendSParameter(u8 param);
	void SendSCommand(u8 cmd);
	u32 ReadSCommandResponse();

	enum S_COMMANDS {
		Subcommand		  = 0x03,
		UpdateStickyFlags = 0x05,
		ReadRTC           = 0x08,
		OpenConfig        = 0x40,
		ReadConfig        = 0x41
	};

	int sector_buffer_index = 0;
	std::vector<u8> sector_buffer;
	std::queue<u8> n_params;
	u8 n_command = 0;
	int n_response_length = 0;
	inline u8 ReadParam() {
		u8 param = n_params.front();
		n_params.pop();
		return param;
	}
	void SendNParameter(u8 param);
	void SendNCommand(u8 cmd);
	void Break();
	static u32 ReadSectorBuffer(u128 unused, void* cdvdptr);

	enum N_COMMANDS {
		ReadCD  = 0x06,
		ReadDVD = 0x08
	};

	bool read_queued = false;
	int counter = 0;
	u32 loc = 0;
	unsigned int block_size = 0;
	u32 n = 0;
	void QueueRead();
	void CDReadSector();
	void DVDReadSector();
	enum class queued_t {
		CD,
		DVD
	};
	queued_t queued;
};