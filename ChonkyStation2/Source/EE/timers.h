#pragma once

#include "../common.h"
#define HBLANK 9370

class Timer {
public:
	u16 counter = 0;
	u16 internal_counter = 0;

	u16 mode = 0;
	u16 comp = 0;
	u16 hold = 0;

	void step();
};