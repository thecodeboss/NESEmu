#include "NESGame.h"
#include "CPU.h"
using namespace std;

NESGame::NESGame() : MapperNum(0), VRAM(0x2000)
{
	NameTable[0] = NRAM + 0x0000;
	NameTable[1] = NRAM + 0x0400;
	NameTable[2] = NRAM + 0x0000;
	NameTable[3] = NRAM + 0x0400;
	for (int32 i=0; i<0x1000; i++) NRAM[i] = 0;
	for (int32 i=0; i<0x800; i++) ExtraNRAM[i] = 0;
	for (int32 i=0; i<0x2000; i++) WRAM[i] = 0;
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
	if ((Address >> 13) == 3) return WRAM[Address & 0x1FFF];
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
				static uint8 Registers[4] = {0x1F, 0, 0, 0};
				static uint8 Counter = 0, Cache = 0;

				if (Value & 0x80)
				{
					Registers[0] |= 0x0C;
					Cache = Counter = 0;
					MMC1ROM(Registers);
					break;
				}

				Cache |= (Value&1) << (Counter++);

				if (Counter == 5)
				{
					uint8 temp = (Address>>13) - 4;
					Registers[temp] = Cache;
					Cache = Counter = 0;
					switch (temp)
					{
					case 0: MMC1Mirror(Registers); MMC1VROM(Registers); MMC1ROM(Registers); break;
					case 1: MMC1VROM(Registers); MMC1ROM(Registers); break;
					case 2: MMC1VROM(Registers);
					case 3: MMC1ROM(Registers); break;
					}
				}
				break;
			}
		case 2: // Rockman, Castlevania, etc.
			SetROM(0x4000, 0x8000, Value);
			break;
		case 3: // Kage, Solomon's Key, etc.
			Value &= Read(Address); // Simulate bus conflict
			SetVROM(0x2000, 0x0000, (Value&3));
			break;
		case 4: // Super Mario Bros 2, 3, etc.
			{
				static uint8 Registers[8] = {0,2,4,5,6,7,0,1};
				switch (Address & 0xE001)
				{
				case 0x8000:
					if ((Value & 0x40) != (MMC3Cmd & 0x40))
					{
						MMC3ROM(Value, Registers);
					}
					if ((Value & 0x80) != (MMC3Cmd & 0x80))
					{
						MMC3VROM(Value, Registers);
					}
					MMC3Cmd = Value;
					break;
				case 0x8001:
					{
						int32 base = (MMC3Cmd & 0x80) << 5;
						Registers[MMC3Cmd & 7] = Value;
						switch (MMC3Cmd & 7)
						{
						case 0:
							SetVROM(0x400, base ^ 0x000, Value & (~1));
							SetVROM(0x400, base ^ 0x400, Value | 1);
							break;
						case 1:
							SetVROM(0x400, base ^ 0x800, Value & (~1));
							SetVROM(0x400, base ^ 0xC00, Value | 1);
							break;
						case 2:
							SetVROM(0x400, base ^ 0x1000, Value);
							break;
						case 3:
							SetVROM(0x400, base ^ 0x1400, Value);
							break;
						case 4:
							SetVROM(0x400, base ^ 0x1800, Value);
							break;
						case 5:
							SetVROM(0x400, base ^ 0x1C00, Value);
							break;
						case 6:
							if (MMC3Cmd & 0x40) SetROM(0x4000, 0xC000, Value);
							else SetROM(0x4000, 0x8000, Value);
							break;
						case 7:
							SetROM(0x4000, 0xA000, Value);
							break;
						}
						break;
					}
				case 0xA000:
					SetMirrorType((Value & 1) ^ 1);
					break;
				case 0xA001:
					// A001B = Value;
					break;
				case 0xC000:
				case 0xC001:
					break;
				case 0xE000:
					cpu->SetInterrupt(false);
					break;
				case 0xE001:
					cpu->SetInterrupt(true);
					break;
				}
				break;
			}
		case 7: // Some rare games
			SetROM(0x8000, 0x8000, (Value&7));
			NameTable[0] = NameTable[1] = NameTable[2] = NameTable[3] = &NRAM[0x400 * ((Value>>4)&1)];
			break;
		default:
			break;
		}
	}
	else if (Address >= 0x6000)
	{
		WRAM[Address & 0x1FFF] = Value;
	}
	return Read(Address);
}

void NESGame::SetVROM( uint32 Size, uint32 BaseAddress, uint32 Index )
{
	for(uint32 v = static_cast<uint32>(VRAM.size()) + Index * Size, p = BaseAddress / VROMGranularity;
		(p < (BaseAddress + Size) / VROMGranularity) && (p < VROMPages);
		++p, v += VROMGranularity)
			VBanks[p] = &VRAM[v % VRAM.size()];
}

void NESGame::SetROM( uint32 Size, uint32 BaseAddress, uint32 Index )
{
	for(uint32 v = static_cast<uint32>(ROM.size()) + Index * Size, p = BaseAddress / ROMGranularity;
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

void NESGame::Dump()
{
	std::cout << "Game Data" << std::endl;
	std::cout << "NRAM:" << std::endl;
	for (int i=0; i<0x1000; i++)
	{
		PrintHex(NRAM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << "WRAM:" << std::endl;
	for (int i=0; i<0x2000; i++)
	{
		PrintHex(WRAM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << "ROM:" << std::endl;
	for (size_t i=0; i<ROM.size(); i++)
	{
		PrintHex(ROM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << "VRAM:" << std::endl;
	for (size_t i=0; i<VRAM.size(); i++)
	{
		PrintHex(VRAM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
}

void NESGame::MMC1ROM( uint8* Registers )
{
	uint8 offset = Registers[1] & 0x10;
	switch( Registers[0] & 0x0C )
	{
	case 0x00: case 0x04:
		SetROM(0x4000, 0x8000, ((Registers[3] & ~1) + offset));
		SetROM(0x4000, 0xC000, ((Registers[3] & ~1) + offset + 1));
		break;
	case 0x08:
		SetROM(0x4000, 0xC000, (Registers[3] + offset));
		SetROM(0x4000, 0x8000, offset);
		break;
	case 0x0C:
		SetROM(0x4000, 0x8000, (Registers[3] + offset));
		SetROM(0x4000, 0xC000, 0xF + offset);
		break;
	}
}

void NESGame::MMC1VROM( uint8* Registers )
{
	if (Registers[0] & 0x10) {
		SetVROM(0x1000, 0x0000, Registers[1]);
		SetVROM(0x1000, 0x1000, Registers[2]);
	} else {
		SetVROM(0x1000, 0x0000, Registers[1] & 0xFE);
		SetVROM(0x1000, 0x1000, Registers[1] | 1);
	}
}

void NESGame::MMC1Mirror( uint8* Registers )
{
	switch (Registers[0] & 3)
	{
	case 2: SetMirrorType(1); break;
	case 3: SetMirrorType(0); break;
	case 0: SetMirrorType(2); break;
	case 1: SetMirrorType(3); break;
	}
}

void NESGame::SetMirroring( uint8 ctrlByte1 )
{
	if (ctrlByte1 & 8)
	{
		Mirroring = 2;
	}
	else
	{
		Mirroring = ctrlByte1 & 1;
	}

	if (Mirroring == 2)
	{
		SetupMirroring(4, 1, ExtraNRAM);
	}
	else if (Mirroring >= 0x10)
	{
		SetupMirroring(2 + (Mirroring & 1), 1, 0);
	}
	else
	{
		SetupMirroring(Mirroring & 1, (Mirroring & 4) >> 2, 0);
	}
}

void NESGame::SetupMirroring( uint8 m, uint8 hard, uint8* extra )
{
	if (m < 4) {
		MirrorHard = 0;
		SetMirrorType(m);
	} else {
		NameTable[0] = NRAM;
		NameTable[1] = NRAM + 0x400;
		NameTable[2] = extra;
		NameTable[3] = extra + 0x400;
	}
	MirrorHard = hard;
}

void NESGame::SetMirrorType( uint8 t )
{
	if (!MirrorHard) {
		switch (t) {
		case 0: // Horizontal
			NameTable[0] = NameTable[1] = NRAM; NameTable[2] = NameTable[3] = NRAM + 0x400;
			break;
		case 1: // Vertical
			NameTable[0] = NameTable[2] = NRAM; NameTable[1] = NameTable[3] = NRAM + 0x400;
			break;
		case 2: // 0
			NameTable[0] = NameTable[1] = NameTable[2] = NameTable[3] = NRAM;
			break;
		case 3: // 1
			NameTable[0] = NameTable[1] = NameTable[2] = NameTable[3] = NRAM + 0x400;
			break;
		}
	}
}

void NESGame::MMC3ROM( uint8 Value, uint8* Registers )
{
	if (Value & 0x40)
	{
		SetROM(0x4000, 0xC000, Registers[6]);
		SetROM(0x4000, 0x8000, ~1);
	}
	else
	{
		SetROM(0x4000, 0x8000, Registers[6]);
		SetROM(0x4000, 0xC000, ~1);
	}
	SetROM(0x4000, 0xA000, Registers[7]);
	SetROM(0x4000, 0xE000, ~0);
}

void NESGame::MMC3VROM( uint8 Value, uint8* Registers )
{
	uint32 base = (Value & 0x80) << 5;

	SetVROM(0x400, base ^ 0x000, Registers[0] & (~1));
	SetVROM(0x400, base ^ 0x400, Registers[0] | 1);
	SetVROM(0x400, base ^ 0x800, Registers[0] & (~1));
	SetVROM(0x400, base ^ 0xC00, Registers[0] | 1);

	SetVROM(0x400, base ^ 0x1000, Registers[2]);
	SetVROM(0x400, base ^ 0x1400, Registers[3]);
	SetVROM(0x400, base ^ 0x1800, Registers[4]);
	SetVROM(0x400, base ^ 0x1C00, Registers[5]);
}

void NESGame::SetName( char* name )
{
	Name = name;
}

void NESGame::Save()
{
	ofstream OutputFile(Name + ".sav", ios::binary);
	OutputFile.unsetf(ios_base::skipws);
	for (int32 i=0; i<0x2000; i++) OutputFile << WRAM[i];
	OutputFile.close();
}

void NESGame::FindAndLoadSave()
{
	ifstream SaveFile(Name + ".sav", ios::binary);
	if (SaveFile.is_open())
	{
		SaveFile.unsetf(ios_base::skipws);
		for (int32 i=0; i<0x2000; i++) SaveFile >> WRAM[i];
		std::cout << "Loaded saved file from " << Name << ".sav." << std::endl;
		SaveFile.close();
	}
}
