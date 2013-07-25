#ifndef PPU_h__
#define PPU_h__

#include "Global.h"

class PPU
{
	u32 Cycles;
public:
	PPU();
	void Tick();
};

#endif // PPU_h__
