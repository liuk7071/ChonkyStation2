#pragma once

#include "../common.h"
#define HBLANK 9370

class Timer {
public:
	u16 counter = 0;
	int internal_counter = 0;

	u16 mode = 0;
	u16 comp = 0;
	u16 hold = 0;

	void step();
};

template<typename T>
concept ValidTimer = std::same_as<T, u16> || std::same_as<T, u32>;

template<typename T, int tid> 	// Some timers have slightly different behaviour, so we need to know what timer we are
requires ValidTimer<T>
class IOPTimer {
public:
	T counter;
	int internal_counter = 0;

	u16 mode;
	T target;

	void step() {
		auto ext = (mode >> 8) & 1;

		if (!ext) {
			counter += 2;
		}
		else {
			Helpers::Panic("Unimplemented IOP Timer external signal bit\n");
		}
	}
};