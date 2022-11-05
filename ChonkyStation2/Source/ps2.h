#include "Memory/memory.h"
#include "EE/EE.h"
#include "IOP/iop.h"

class Ps2 {
public:
	// Components
	DMAC dma;
	IOPDMA iopdma;
	GS gs;
	GIF gif = GIF(&gs);
	SIF sif;
	Memory memory = Memory(&dma, &iopdma, &gif, &sif, &gs);
	EE ee = EE(&memory);
	IOP iop = IOP(&memory);
};