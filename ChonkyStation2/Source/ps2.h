#include "Memory/memory.h"
#include "EE/EE.h"
#include "IOP/iop.h"
#include "CDVD/cdvd.h"

class Ps2 {
public:
	// Components
	DMAC dma;
	IOPDMA iopdma;
	GS gs;
	GIF gif = GIF(&gs);
	SIF sif;
	CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\Atelier Iris - Eternal Mana (USA) (En,Ja).iso");
	Memory memory = Memory(&dma, &iopdma, &gif, &sif, &gs, &cdvd);
	EE ee = EE(&memory);
	IOP iop = IOP(&memory);
};