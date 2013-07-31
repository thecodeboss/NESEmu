#ifndef NESGame_h__
#define NESGame_h__

#include "Global.h"
#include <vector>

class NESGame
{
	uint8 mappernum;
	std::vector<uint8> ROM, VRAM;

public:
	NESGame();
	void SetMapperNum(uint8 in);
	void SetROMSize( uint8 numROM16 );
	void SetVRAMSize( uint8 numROM16 );
	void LoadROM(std::ifstream& InputGame);
	void LoadVRAM(std::ifstream& InputGame);
};

#endif // NESGame_h__
