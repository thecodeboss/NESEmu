#include "NESGame.h"
using namespace std;

NESGame::NESGame() : mappernum(0)
{

}

void NESGame::SetMapperNum( uint8 in )
{
	mappernum = in;
}

void NESGame::SetROMSize( uint8 numROM16 )
{
	ROM.resize(numROM16 * 0x4000);
}

void NESGame::SetVRAMSize( uint8 numROM16 )
{
	VRAM.resize(numROM16 * 0x2000);
}

void NESGame::LoadROM(std::ifstream& InputGame)
{
	for (uint32 i=0; i < ROM.size() && InputGame >> ROM[i]; i++);
}

void NESGame::LoadVRAM(std::ifstream& InputGame)
{
	for (uint32 i=0; i < VRAM.size() && InputGame >> VRAM[i]; i++);
}
