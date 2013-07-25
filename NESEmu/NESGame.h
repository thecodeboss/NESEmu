#ifndef NESGame_h__
#define NESGame_h__

#include "Global.h"
#include <vector>

class NESGame
{
	u8 mappernum;
	std::vector<u8> ROM, VRAM;

public:
	NESGame();
	void SetMapperNum(u8 in);
	void SetROMSize( u8 numROM16 );
	void SetVRAMSize( u8 numROM16 );
	void LoadROM(std::ifstream& InputGame);
	void LoadVRAM(std::ifstream& InputGame);
};

#endif // NESGame_h__
