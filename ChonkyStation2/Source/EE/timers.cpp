#include "timers.h"

void Timer::step() {
	u16 old = counter;	// To check for overflow
	if (!((mode >> 7) & 1)) return;

	// Currently only HBLANK
	internal_counter++;
	if (internal_counter > HBLANK) {
		internal_counter = 0;
		counter++;
	}

	if ((mode >> 8) & 1) {	// Compare interrupt
		if (counter == comp) {
			mode |= 1 << 10;
			if ((mode >> 6) & 1) {	// Clear if comp is reached
				counter = 0;
			}
		}
	}

	if ((mode >> 9) & 1) {	// Overflow interrupt
		if (counter < old) {	// We overflowed
			mode |= 1 << 11;
		}
	}

	if ((mode >> 10) & 3) {
		*intc_stat |= 1 << (9 + id);
	}
}