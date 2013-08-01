#include "PPU.h"

PPU::PPU() : Cycles(0), OpenBus(0), OpenBusDecayTimer(0), VBlankState(0), ReadBuffer(0), OffsetToggle(false)
{
	
}

void PPU::Tick()
{
	Cycles++;
}

uint8 PPU::Read( uint16 Address )
{
	uint8 temp, RetValue = OpenBus;
	switch(Address)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		RetValue = PPUREG.PPUSTATUS | (OpenBus & 0x1F);
		PPUREG.VBlankStarted = false; // Clear the VBlank flag since reading $2002
		OffsetToggle = false;
		if (VBlankState != -5) VBlankState = 0;
		break;
	case 3:
		break;
	case 4:
		temp = OAM[PPUREG.OAMADDR] & (PPUREG.OAMData == 2 ? 0xE3 : 0xFF);
		RetValue = RefreshOpenBus(temp);
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		RetValue = ReadBuffer;
		temp = MemoryMap(PPUADDR.raw);
		if ((PPUADDR.raw & 0x3F00) == 0x3F00)
		{
			RetValue = ReadBuffer = (OpenBus & 0xC0) | (temp & 0x3F);
		}
		ReadBuffer = temp;
		RefreshOpenBus(RetValue);
		PPUADDR.raw = PPUADDR.raw + (PPUREG.AddressIncrement ? 32 : 1);
		break;
	default:
		break;
	}
	return RetValue;
}

uint8 PPU::Write( uint16 Address, uint8 Value )
{
	uint8 temp, RetValue = OpenBus;
	switch(Address)
	{
	case 0:
		PPUREG.PPUCTRL = Value;
		PPUSCROLL.NameTableAddress = PPUREG.BaseNameTableAddress;
		break;
	case 1:
		PPUREG.PPUMASK = Value;
		break;
	case 2:
		break;
	case 3:
		PPUREG.OAMADDR = Value;
		break;
	case 4:
		OAM[PPUREG.OAMADDR++] = Value;
		break;
	case 5:
		if (OffsetToggle)
		{
			PPUSCROLL.YFine = Value & 7;
			PPUSCROLL.YCoarse = Value >> 3;
		}
		else PPUSCROLL.XScroll = Value;
		OffsetToggle ^= true;
		break;
	case 6:
		if (OffsetToggle)
		{
			PPUSCROLL.VAddrLo = Value;
			PPUADDR.raw = PPUSCROLL.raw;
		}
		else PPUSCROLL.VAddrHi = Value & 0x3F;
		OffsetToggle ^= true;
		break;
	case 7:
		RetValue = ReadBuffer;
		temp = MemoryMap(PPUADDR.raw);
		RetValue = temp = Value;
		RefreshOpenBus(RetValue);
		PPUADDR.raw = PPUADDR.raw + (PPUREG.AddressIncrement ? 32 : 1);
		break;
	default:
		break;
	}
	return RetValue;
}

uint8 PPU::MemoryMap(uint32 raw )
{
	raw &= 0x3FFF;
	if (raw >= 0x3F00)
	{
		if (raw%4 == 0) raw &= 0x0F;
		return palette[raw & 0x1F];
	}
	else if (raw < 0x2000)
	{
		uint32 i = (raw / game->VROMGranularity) % game->VROMPages;
		uint32 j = raw % game->VROMGranularity;
		return game->VBanks[i][j];
	}
	else return game->NameTable[(raw>>10)&3][raw&0x3FF];
}

uint8 PPU::RefreshOpenBus( uint8 RetValue )
{
	return OpenBusDecayTimer = 77777, OpenBus = RetValue;
}

void PPU::SetGame( NESGame* g )
{
	game = g;
}

