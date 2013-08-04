#include "NESGame.h"
using namespace std;

NESGame::NESGame() : MapperNum(0), VRAM(0x2000)
{
	NameTable[0] = NRAM + 0x0000;
	NameTable[1] = NRAM + 0x0400;
	NameTable[2] = NRAM + 0x0000;
	NameTable[3] = NRAM + 0x0400;
}

void NESGame::SetMapperNum( uint8 in )
{
	if (in >= 0x40) in &= 15;
	MapperNum = in;
}

void NESGame::SetROMSize( uint8 numROM16 )
{
	if (numROM16) ROM.resize(numROM16 * 0x4000);
}

void NESGame::SetVRAMSize( uint8 numVROM8 )
{
	if (numVROM8) VRAM.resize(numVROM8 * 0x2000);
}

void NESGame::LoadROM(std::ifstream& InputGame)
{
	for (uint32 i=0; i < ROM.size(); i++) InputGame >> ROM[i];
}

void NESGame::LoadVRAM(std::ifstream& InputGame)
{
	for (uint32 i=0; i < VRAM.size(); i++) InputGame >> VRAM[i];
}

uint8 NESGame::Read( uint32 Address )
{
	if ((Address >> 13) == 3) return PRAM[Address & 0x1FFF];
	return Banks[ (Address / ROMGranularity) % ROMPages][Address % ROMGranularity];
}

uint8 NESGame::Write( uint32 Address, uint8 Value )
{
	if (Address >= 0x8000)
	{
		switch (MapperNum)
		{
		case 1: // Rockman 2, Simon's Quest, etc.
			{
				static uint8 Registers[4] = {0x0C, 0, 0, 0};
				static uint8 Counter = 0, Cache = 0;
				if (!(Value & 0x80))
				{
					Cache |= (Value&1) << Counter;
					if (++Counter != 5) break;
					Registers[(Address>>13) & 3] = Value = Cache;
				}
				else Registers[0] = 0x0C;
				Cache = Counter = 0;
				static const uint8 Selector[4][4] = { {0,0,0,0}, {1,1,1,1}, {0,1,0,1}, {0,0,1,1} };
				for (uint32 m=0; m<4; ++m)
				{
					NameTable[m] = &NRAM[0x400 * Selector[Registers[0]&3][m]];
				}
				SetVROM(0x1000, 0x0000, ((Registers[0]&16) ? Registers[1] : ((Registers[1]&~1)+0)));
				SetVROM(0x1000, 0x1000, ((Registers[0]&16) ? Registers[2] : ((Registers[1]&~1)+1)));
				switch( (Registers[0]>>2)&3 )
				{
				case 0: case 1:
					SetROM(0x8000, 0x8000, (Registers[3] & 0xE) / 2);
					break;
				case 2:
					SetROM(0x4000, 0x8000, 0);
					SetROM(0x4000, 0xC000, (Registers[3] & 0xF));
					break;
				case 3:
					SetROM(0x4000, 0x8000, (Registers[3] & 0xF));
					SetROM(0x4000, 0xC000, ~0);
					break;
				}
			}
		case 2: // Rockman, Castlevania, etc.
			SetROM(0x4000, 0x8000, Value);
			break;
		case 3: // Kage, Solomon's Key, etc.
			Value &= Read(Address); // Simulate bus conflict
			SetVROM(0x2000, 0x0000, (Value&3));
			break;
		case 7: // Some rare games
			SetROM(0x8000, 0x8000, (Value&7));
			NameTable[0] = NameTable[1] = NameTable[2] = NameTable[3] = &NRAM[0x400 * ((Value>>4)&1)];
			break;
		default:
			break;
		}
	}
	return Read(Address);
}

void NESGame::SetVROM( uint32 Size, uint32 BaseAddress, uint32 Index )
{
	for(uint32 v = VRAM.size() + Index * Size, p = BaseAddress / VROMGranularity;
		(p < (BaseAddress + Size) / VROMGranularity) && (p < VROMPages);
		++p, v += VROMGranularity)
			VBanks[p] = &VRAM[v % VRAM.size()];
}

void NESGame::SetROM( uint32 Size, uint32 BaseAddress, uint32 Index )
{
	for(uint32 v = ROM.size() + Index * Size, p = BaseAddress / ROMGranularity;
		(p < (BaseAddress + Size) / ROMGranularity) && (p < ROMPages);
		++p, v += ROMGranularity)
			Banks[p] = &ROM[v % ROM.size()];
}

void NESGame::Init()
{
	for (uint32 i=0; i<ROMPages; i++) Banks[i] = NULL;
	for (uint32 i=0; i<VROMPages; i++) VBanks[i] = NULL;
	SetVROM(0x2000, 0x0000, 0);
	for(uint32 v=0; v<4; ++v) SetROM(0x4000, v*0x4000, v==3 ? -1 : 0);
}
