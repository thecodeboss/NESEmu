#include "Global.h"
#include "CPU.h"
#include "NESGame.h"
#include "IO.h"
using namespace std;

char ToHex(uint8 in)
{
	if (in < 10) return in + '0';
	else return (in-10) + 'A';
}

void PrintHex(uint8 in)
{
	cout << ToHex(in >> 4) << ToHex(in & 0x0F) << " ";
	//cout << (int)in << " ";
}

void ReadHex(ifstream& input, uint8& output) 
{
	input >> output;
	//PrintHex(output);
}

int main(int argc, char** argv)
{
	IO* io = new IO();
	APU* apu = new APU();
	PPU* ppu = new PPU();
	NESGame* game = new NESGame();
	CPU* cpu = new CPU();

	io->Init(2, 2);
	io->SetNTSCMode(false);

	if (argc > 2 && !strcmp(argv[2],"framedump")) io->SetFrameDump(true);

	cpu->SetIO(io);
	cpu->SetAPU(apu);
	cpu->SetPPU(ppu);
	cpu->SetGame(game);
	cpu->Init();

	ppu->SetIO(io);
	ppu->SetGame(game);
	ppu->Init();

	apu->SetCPU(cpu);

	ifstream InputGame(argv[1], ios::binary);
	InputGame.unsetf(ios_base::skipws);

	uint8 temp, numROM16, numVROM8, ctrlByte1, ctrlByte2;
	for(int i=0;i<4;i++) ReadHex(InputGame, temp); // Read 'NES^Z' from header

	//InputGame >> numROM16 >> numVROM8 >> ctrlByte1 >> ctrlByte2;
	ReadHex(InputGame, numROM16);
	ReadHex(InputGame, numVROM8);
	ReadHex(InputGame, ctrlByte1);
	ReadHex(InputGame, ctrlByte2);

	for(int i=0;i<8;i++) ReadHex(InputGame, temp); // Read 8 junk bytes

	game->SetMapperNum(ctrlByte2 | (ctrlByte1 >> 4));
	game->SetROMSize(numROM16);
	game->SetVRAMSize(numVROM8);
	game->LoadROM(InputGame);
	game->LoadVRAM(InputGame);

	game->Init();

	cpu->Run();

	cpu->Dump();
	ppu->Dump();
	apu->Dump();
	game->Dump();

	return 0;
}
