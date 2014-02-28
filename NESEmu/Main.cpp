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
	if (argc < 2)
	{
		cout << "You need to run this application with an input NES game." << endl;
		cout << "Format is:" << endl;
		cout << "NESEmu path/to/game.nes" << endl;
		return 0;
	}

	ifstream InputGame(argv[1], ios::binary);
	if (!InputGame.is_open())
	{
		cout << "Failed to open input file; check your path." << endl;
		return 0;
	}

	IO* io = new IO();
	APU* apu = new APU();
	PPU* ppu = new PPU();
	NESGame* game = new NESGame();
	CPU* cpu = new CPU();

	io->Init(2, 2);
	io->SetNTSCMode(false);
	io->SetFPS(60.0f);

	if (argc > 2 && !strcmp(argv[2],"framedump")) io->SetFrameDump(true);
	if (argc > 3 && !strcmp(argv[3],"audiodump")) io->SetAudioDump(true);

	cpu->SetIO(io);
	cpu->SetAPU(apu);
	cpu->SetPPU(ppu);
	cpu->SetGame(game);
	cpu->Init();

	ppu->SetIO(io);
	ppu->SetGame(game);
	ppu->Init();

	apu->SetIO(io);
	apu->SetCPU(cpu);

	InputGame.unsetf(ios_base::skipws);

	uint8 temp, numROM16, numVROM8, ctrlByte1, ctrlByte2;
	for(int i=0;i<4;i++) ReadHex(InputGame, temp); // Read 'NES^Z' from header

	ReadHex(InputGame, numROM16);
	ReadHex(InputGame, numVROM8);
	ReadHex(InputGame, ctrlByte1);
	ReadHex(InputGame, ctrlByte2);

	for(int i=0;i<8;i++) ReadHex(InputGame, temp); // Read 8 junk bytes

	game->SetName(argv[1]);
	game->SetMapperNum(ctrlByte2 | (ctrlByte1 >> 4));
	game->SetMirroring(ctrlByte1);
	game->SetROMSize(numROM16);
	game->SetVRAMSize(numVROM8);
	game->LoadROM(InputGame);
	game->LoadVRAM(InputGame);
	game->Init();
	game->FindAndLoadSave();
	InputGame.close();

	cpu->Run();

	// If audiodump is set, write out a wav file
	// Hardcoded since rarely used, and don't feel like dealing with
	// missing/incorrect directories.
	io->WriteWAV("output.wav");

	delete cpu;
	delete game;
	delete ppu;
	delete apu;
	delete io;

	return 0;
}
