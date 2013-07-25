#include "CPU.h"

u8 CPU::Read(u16 Address)
{
	Tick();
	if (Address < 0x2000) return RAM[Address&0x7FF];
	else return 0; // TODO
}

u8 CPU::Write(u16 Address, u8 Value)
{
	Tick();
	if (Address < 0x2000) RAM[Address&0x7FF] = Value;
	// else = TODO
	return 0;
}

u16 CPU::WrapAddress(u16 Old, u16 New)
{
	return (Old & 0xFF00) + u8(New);
}

void CPU::Misfire(u16 Old, u16 New)
{
	u16 temp = WrapAddress(Old, New);
	if (temp != New) Read(New);
}

u8 CPU::Pop()
{
	return Read(0x100 | u8(++SP));
}

void CPU::Push(u8 value)
{
	Write(0x100 | u8(SP--), value);
}

void CPU::Tick()
{
	// PPU clock speed is 3x the CPU clock speed
	for (u32 i=0; i<3; ++i) ppu->Tick();

	// APU clock speed is the same as the CPU clock speed
	apu->Tick();
	Cycles++;
}

#define c(n) Ins[0x##n]=&CPU::Instruction<0x##n>;Ins[0x##n+1]=&CPU::Instruction<0x##n+1>;
#define o(n) c(n)c(n+2)c(n+4)c(n+6)
CPU::CPU() : A(0), X(0), Y(0), SP(0), PC(0xC000), Cycles(0), Reset(false), NMI(false), NMIEdgeDetected(false), Interrupt(false), apu(NULL), ppu(NULL)
{
	o(00)o(08)o(10)o(18)o(20)o(28)o(30)o(38)
	o(40)o(48)o(50)o(58)o(60)o(68)o(70)o(78)
	o(80)o(88)o(90)o(98)o(A0)o(A8)o(B0)o(B8)
	o(C0)o(C8)o(D0)o(D8)o(E0)o(E8)o(F0)o(F8) o(100)
}
#undef o
#undef c

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
	// Read the next instruction
	u16 op = Read(PC++);

	if (Reset) op = 0x101; // Force reset
	else if (NMI && !NMIEdgeDetected)
	{
		op = 0x100;
		NMIEdgeDetected = true;
	}
	else if (Interrupt && !P.I) op = 0x102;
	if (!NMI) NMIEdgeDetected = false;

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
