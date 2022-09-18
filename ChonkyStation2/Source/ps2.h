#include "Memory/memory.h"
#include "EE/EE.h"

class Ps2 {
public:
	// Components
	DMA dma;
	GS gs;
	GIF gif = GIF(&gs);
	Memory memory = Memory(&dma, &gif, &gs);
	EE ee = EE(&memory);
};