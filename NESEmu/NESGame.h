#ifndef NESGame_h__
#define NESGame_h__

#include "Global.h"
#include <vector>

class NESGame
{
public:
	uint8 MapperNum;
	std::vector<uint8> ROM, VRAM;
	uint8 NRAM[0x1000], PRAM[0x2000];
	static const uint32 VROMGranularity = 0x0400;
	static const uint32 ROMGranularity = 0x2000;
	static const uint32 VROMPages = 0x2000 / VROMGranularity;
	static const uint32 ROMPages = 0x10000 / ROMGranularity;

	uint8* Banks[ROMPages];
	uint8* VBanks[VROMPages];
	uint8* NameTable[4];

	NESGame();
	void SetMapperNum(uint8 in);
	void SetROMSize( uint8 numROM16 );
	void SetVRAMSize( uint8 numVROM8 );
	void LoadROM(std::ifstream& InputGame);
	void LoadVRAM(std::ifstream& InputGame);
	uint8 Read( uint32 Address );
	uint8 Write( uint32 Address, uint8 Value );
	void SetVROM( uint32 Size, uint32 BaseAddress, uint32 Index );
	void SetROM( uint32 Size, uint32 BaseAddress, uint32 Index );
	void Init();
};

#endif // NESGame_h__
