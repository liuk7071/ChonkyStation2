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

	int id = 0;	// To know what timer we are, used to set the appropiate interrupt in INTC_STAT
	u16* intc_stat;
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

	bool irq = false;	// Used to check if we need to raise the bit in I_STAT

	void step() {
		auto ext = (mode >> 8) & 1;

		auto irq_enabled = (mode >> 10) & 1;
		auto compare_enabled = (mode >> 4) & 1;
		auto overflow_enabled = (mode >> 5) & 1;

		if (!ext) {
			counter++;

			if (irq_enabled) {
				if (compare_enabled) {
					if (counter == target) {
						irq = true;
						mode |= 1 << 11;
					}
				}
				if (overflow_enabled) {
					T overflow = 0;
					overflow--;
					if (counter == overflow) {
						irq = true;
						mode |= 1 << 12;
					}
				}
			}
		}
		else {
			//Helpers::Panic("Unimplemented IOP Timer external signal bit\n");
		}
	}
};