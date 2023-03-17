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
	//CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\Gran Turismo 4 (USA).iso");
	//CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\Need for Speed - Most Wanted (USA) (v2.00).iso");
	//CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\Crash - Mind Over Mutant (USA).iso");
	//CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\Planetarian - Chiisa na Hoshi no Yume (Japan).iso");
	//CDVD cdvd = CDVD("H:\\Games\\roms\\PS2\\FIFA Football 2005 (Europe) (En,Nl,Sv,No,Da,El).iso");
	Memory memory = Memory(&dma, &iopdma, &gif, &sif, &gs, &cdvd);
	EE ee = EE(&memory);
	IOP iop = IOP(&memory);
};