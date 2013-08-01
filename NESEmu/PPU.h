#ifndef PPU_h__
#define PPU_h__

#include "Global.h"
#include "NESGame.h"

class PPU
{
	uint64 Cycles;
	int32 OpenBus, OpenBusDecayTimer, VBlankState, ReadBuffer;
	uint8 OAM[256], palette[32];
	bool OffsetToggle;
	NESGame* game;

	union Register
	{
		uint32 raw;

		// Register 0 (write)
		Bit<0,8,uint32> PPUCTRL; // Various flags controlling PPU operation
		Bit<0,2,uint32> BaseNameTableAddress; // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
		Bit<2,1,uint32> AddressIncrement; // (0: add 1, going across; 1: add 32, going down)
		Bit<3,1,uint32> SpritePatternTableAddress; // (0: $0000; 1: $1000; ignored in 8x16 mode)
		Bit<4,1,uint32> BackgroundPatternTableAddress; // (0: $0000; 1: $1000)
		Bit<5,1,uint32> SpriteSize; // (0: 8x8; 1: 8x16)
		Bit<6,1,uint32> SlaveFlag; // (0: read backdrop from EXT pins; 1: output color on EXT pins)
		Bit<7,1,uint32> NMIEnabled; // (0: off; 1: on)

		// Register 1 (write)
		Bit<8,8,uint32> PPUMASK; // This register controls screen enable, masking, and intensity
		Bit<8,1,uint32> Grayscale; // (0: normal color; 1: produce a monochrome display)
		Bit<9,1,uint32> ShowBG8; // 1: Show background in leftmost 8 pixels of screen; 0: Hide
		Bit<10,1,uint32> ShowSP8; // 1: Show sprites in leftmost 8 pixels of screen; 0: Hide
		Bit<11,1,uint32> ShowBG; // 1: Show background
		Bit<12,1,uint32> ShowSP; // 1: Show sprites
		Bit<13,1,uint32> IntensifyReds; // 1: Intensify
		Bit<14,1,uint32> IntensifyGreens; // 1: Intensify
		Bit<15,1,uint32> IntensifyBlues; // 1: Intensify

		// Register 2 (read)
		Bit<8,8,uint32> PPUSTATUS; // This register reflects the state of various functions inside the PPU
		Bit<21,1,uint32> SpriteOverflow; // It's complicated
		Bit<22,1,uint32> Sprite0Hit; // Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel
		Bit<23,1,uint32> VBlankStarted; // (0: not in VBLANK; 1: in VBLANK)

		// Register 3 (write)
		Bit<24,8,uint32> OAMADDR; // Somewhat obscure
		Bit<24,2,uint32> OAMData;
		Bit<26,6,uint32> OAMIndex;
	} PPUREG;

	union ScrollType
	{
		Bit<3,16,uint32> raw;
		Bit<0,8,uint32> XScroll;
		Bit<0,3,uint32> XFine;
		Bit<3,5,uint32> XCoarse;
		Bit<8,5,uint32> YCoarse;
		Bit<13,2,uint32> NameTableAddress;
		Bit<13,1,uint32> HorizNameTableIndex;
		Bit<14,1,uint32> VertNameTableIndex;
		Bit<15,3,uint32> YFine;
		Bit<11,8,uint32> VAddrHi;
		Bit<3,8,uint32> VAddrLo;
	} PPUSCROLL, PPUADDR;
public:
	PPU();
	void Tick();
	uint8 Read( uint16 Address );
	uint8 Write( uint16 Address, uint8 Value);
	uint8 MemoryMap( uint32 raw );
	uint8 RefreshOpenBus( uint8 RetValue );
	void SetGame( NESGame* g );
};

#endif // PPU_h__
