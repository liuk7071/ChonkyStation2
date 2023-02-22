#include "timers.h"

void Timer::step() {
	// Currently only HBLANK

	internal_counter++;
	if (internal_counter > HBLANK) {
		internal_counter = 0;
		counter++;
	}
}