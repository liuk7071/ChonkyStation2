#include "Memory/memory.h"
#include "EE/EE.h"

class Ps2 {
public:
	// Components
	DMA dma;
	GS gs;
	Memory memory = Memory(&dma, &gs);
	EE ee = EE(&memory);
};