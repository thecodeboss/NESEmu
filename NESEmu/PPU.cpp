#include "PPU.h"

PPU::PPU() : Cycles(0)
{

}

void PPU::Tick()
{
	Cycles++;
}

uint8 PPU::Read( uint16 Address )
{
	uint8 RetValue;
	switch(Address)
	{
	case 0:
	case 1:
		break;
	case 2:
		break;
	default:
		break;
	}
	return 0; // TODO
}

