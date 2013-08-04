#ifndef APU_h__
#define APU_h__

#include "Global.h"
class CPU;

class APU
{
	uint64 Cycles;
	CPU* cpu;
public:
	APU();
	void Tick();
	void Dump();
	void SetCPU(CPU* c);
	void Write(uint8 index, uint8 value);
	uint8 Read();
};

#endif // APU_h__
