#include "CPU.h"

uint8 CPU::Read(uint16 Address)
{
	Tick();
	if (Address < 0x2000) return RAM[Address&0x7FF];
	else if (Address < 0x4000) return ppu->Read(Address&7);
	else if (Address < 0x4018)
	{
		switch (Address & 0x1F)
		{
		case 0x14:
			return 0;
		case 0x15:
			return 0; // APU ----- TODO
		case 0x16:
			return io->ReadJoystick(0);
		case 0x17:
			return io->ReadJoystick(1);
		default:
			return 0;
			break;
		}
	}
	else return game->Read(Address);
}

uint8 CPU::Write(uint16 Address, uint8 Value)
{
	if (Reset) return Read(Address);
	Tick();
	if (Address < 0x2000) RAM[Address&0x7FF] = Value;
	else if (Address < 0x4000) return ppu->Write(Address&7, Value);
	else if (Address < 0x4018)
	{
		switch (Address & 0x1F)
		{
		case 0x14:
			for(uint32 b=0; b<256; ++b) Write(0x2004, Read((Value&7)*0x0100+b));
			break;
		case 0x15:
			break; // APU TODO
		case 0x16:
			io->StrobeJoystick(Value);
			break;
		case 0x17: // APU
		default:
			break;
		}
	}
	else return game->Write(Address, Value);

	return 0;
}

uint16 CPU::WrapAddress(uint16 Old, uint16 New)
{
	return (Old & 0xFF00) + uint8(New);
}

void CPU::Misfire(uint16 Old, uint16 New)
{
	uint16 temp = WrapAddress(Old, New);
	if (temp != New) Read(New);
}

uint8 CPU::Pop()
{
	return Read(0x100 | uint8(++SP));
}

void CPU::Push(uint8 value)
{
	Write(0x100 | uint8(SP--), value);
}

void CPU::Tick()
{
	Cycles++;
	// PPU clock speed is 3x the CPU clock speed
	for (uint32 i=0; i<3; ++i) ppu->Tick();

	// APU clock speed is the same as the CPU clock speed
	apu->Tick();
}

#define c(n) Ins[0x##n]=&CPU::Instruction<0x##n>;Ins[0x##n+1]=&CPU::Instruction<0x##n+1>;
#define o(n) c(n)c(n+2)c(n+4)c(n+6)
CPU::CPU() : A(0), X(0), Y(0), SP(0), PC(0xC000), Cycles(0), Reset(true), NMI(false), NMIEdgeDetected(false), Interrupt(false), io(NULL), apu(NULL), ppu(NULL)
{
	o(00)o(08)o(10)o(18)o(20)o(28)o(30)o(38)
	o(40)o(48)o(50)o(58)o(60)o(68)o(70)o(78)
	o(80)o(88)o(90)o(98)o(A0)o(A8)o(B0)o(B8)
	o(C0)o(C8)o(D0)o(D8)o(E0)o(E8)o(F0)o(F8) o(100)
	P.raw = 0;
}
#undef o
#undef c

void CPU::Init()
{
	for (uint32 i=0; i<0x800; ++i) RAM[i] = (i&4) ? 0xFF : 0x00;
}

void CPU::Run()
{
	io->StartClock();
	while (io->Poll() /*&& Cycles < 2000000*/)
	{
		Op();
	}
}

void CPU::SetAPU( APU* in )
{
	apu = in;
}

void CPU::SetPPU( PPU* in )
{
	ppu = in;
}

bool CPU::Op()
{
	// Set the NMI value
	bool NMINow = ppu->GetNMI();

	// Read the next instruction
	uint32 op = Read(PC++);

	if (Reset) op = 0x101; // Force reset
	else if (NMINow && !NMIEdgeDetected)
	{
		op = 0x100;
		NMIEdgeDetected = true;
	}
	else if (Interrupt && !P.I) op = 0x102;
	if (!NMINow) NMIEdgeDetected = false;

	// Execute the instruction
	(this->*Ins[op])();

	// In case we did a reset, reset the reset flag
	Reset = false;

	return (op != 0);
}

void CPU::Print()
{
	std::cout << "A = " << std::hex << (int)A << "; X = " << (int)X << "; Y = " << (int)Y << "; PC = " << PC << std::endl;
	std::cout << "RAM:" << std::endl;
	for (int i=0; i<15; i++) {
		std::cout << i << ": " << (int)RAM[i] << std::endl;
	}
	std::cout << "Flags: " << std::bitset<8>(P.raw) << std::endl;
	std::cout << std::endl;
}

void CPU::SetIO( IO* i )
{
	io = i;
}

void CPU::SetGame( NESGame* g )
{
	game = g;
}

void CPU::Dump()
{
	std::cout << "CPU Data" << std::endl;
	std::cout << "A = " << std::hex << (int)A << "; X = " << (int)X << "; Y = " << (int)Y << "; PC = " << PC << std::endl;
	std::cout << "Flags: " << std::bitset<8>(P.raw) << std::endl;
	std::cout << "RAM:" << std::endl;
	for (int i=0; i<0x800; i++)
	{
		PrintHex(RAM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

bool CPU::SetInterrupt( bool b )
{
	return Interrupt = b;
}
