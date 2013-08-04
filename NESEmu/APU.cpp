#include "APU.h"
#include "CPU.h"

APU::APU() : Cycles(0)
{

}

void APU::Tick()
{
	Cycles++;
}

void APU::Dump()
{

}

void APU::SetCPU( CPU* c )
{
	cpu = c;
}

uint8 APU::Read()
{
	return 0;
}

void APU::Write( uint8 index, uint8 value )
{

}
