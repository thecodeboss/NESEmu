#include "CPU.h"
#include "NESGame.h"
#include "IO.h"
using namespace std;

int main(int argc, char** argv)
{
	IO* io = new IO();
	APU* apu = new APU();
	PPU* ppu = new PPU();
	NESGame* game = new NESGame();
	CPU* cpu = new CPU();

	io->Init();
	cpu->SetAPU(apu);
	cpu->SetPPU(ppu);
	ppu->SetIO(io);
	ppu->SetGame(game);

	ifstream InputGame(argv[1]);

	uint8 temp, numROM16, numVROM8, ctrlByte1, ctrlByte2;
	for(int i=0;i<4;i++) InputGame >> temp; // Read 'NES^Z' from header

	InputGame >> numROM16 >> numVROM8 >> ctrlByte1 >> ctrlByte2;

	for(int i=0;i<7;i++) InputGame >> temp; // Read 7 junk bytes

	game->SetMapperNum(ctrlByte2 | (ctrlByte1 >> 4));
	game->SetROMSize(numROM16);
	game->SetVRAMSize(numVROM8);
	game->LoadROM(InputGame);
	game->LoadVRAM(InputGame);

	cpu->Init();
	cpu->Run();

	return 0;
}
