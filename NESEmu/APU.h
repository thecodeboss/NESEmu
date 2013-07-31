#ifndef APU_h__
#define APU_h__

#include "Global.h"

class APU
{
	uint64 Cycles;
public:
	APU();
	void Tick();
};

#endif // APU_h__
