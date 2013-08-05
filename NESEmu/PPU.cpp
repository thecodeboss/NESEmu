#include "PPU.h"

PPU::PPU() : Cycles(0)
	, OpenBus(0)
	, OpenBusDecayTimer(0)
	, VBlankState(0)
	, ReadBuffer(0)
	, x(0)
	, Scanline(241)
	, CycleCounter(0)
	, ScanlineEnd(341)
	, OffsetToggle(false)
	, NMI(false)
	, NMIWasSet(false)
	, NMIWasRead(false)
	, EvenOddToggle(false)
{

}

void PPU::Tick()
{
	Cycles++;

	// Handle VBlank
	switch (VBlankState)
	{
	case -5:
		PPUREG.PPUSTATUS = 0;
		break;
	case 0:
		NMI = PPUREG.VBlankStarted && PPUREG.NMIEnabled;
		if (NMI) NMIWasSet = true;
		else if (!NMIWasRead) NMIWasSet = false;
		break;
	case 2:
		PPUREG.VBlankStarted = true;
		break;
	default:
		break;
	}
	if (VBlankState != 0) VBlankState += (VBlankState < 0 ? 1 : -1);
	if (OpenBusDecayTimer && !--OpenBusDecayTimer) OpenBus = 0;

	// Process graphics/rendering
	if (Scanline < 240)
	{
		if (PPUREG.ShowBG || PPUREG.ShowSP) RenderTick();
		if (Scanline >= 0 && x < 256) RenderPixel();
	}

	// Check for end of scanline
	if (++CycleCounter == 3) CycleCounter = 0;
	if (++x >= ScanlineEnd)
	{
		// Begin a new scanline
		io->FlushScanline(Scanline);
		ScanlineEnd = 341;
		x = 0;

		// Check if the scanline has reached a special state
		switch (Scanline += 1)
		{
		case 241: // Vertical blanking begins
			
			VBlankState = 2;
			break;
		case 261: // Begin rendering
			Scanline = -1;
			EvenOddToggle = !EvenOddToggle;
			VBlankState = -5;
			break;
		default:
			break;
		}
	}

}

uint8 PPU::Read( uint16 Address )
{
	uint8 temp, RetValue = OpenBus;
	switch(Address)
	{
	case 0:
		break;
	case 1:
		break;
	case 2:
		RetValue = PPUREG.PPUSTATUS | (OpenBus & 0x1F);
		PPUREG.VBlankStarted = false; // Clear the VBlank flag since reading $2002
		OffsetToggle = false;
		if (VBlankState != -5) VBlankState = 0;
		break;
	case 3:
		break;
	case 4:
		temp = OAM[PPUREG.OAMADDR] & (PPUREG.OAMData == 2 ? 0xE3 : 0xFF);
		RetValue = RefreshOpenBus(temp);
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		RetValue = ReadBuffer;
		temp = MemoryMap(PPUADDR.raw);
		if ((PPUADDR.raw & 0x3F00) == 0x3F00)
		{
			RetValue = ReadBuffer = (OpenBus & 0xC0) | (temp & 0x3F);
		}
		ReadBuffer = temp;
		RefreshOpenBus(RetValue);
		PPUADDR.raw = PPUADDR.raw + (PPUREG.AddressIncrement ? 32 : 1);
		break;
	default:
		break;
	}
	return RetValue;
}

uint8 PPU::Write( uint16 Address, uint8 Value )
{
	uint8 RetValue = OpenBus;
	RefreshOpenBus(Value);
	switch(Address)
	{
	case 0:
		PPUREG.PPUCTRL = Value;
		PPUSCROLL.NameTableAddress = PPUREG.BaseNameTableAddress;
		break;
	case 1:
		PPUREG.PPUMASK = Value;
		break;
	case 2:
		break;
	case 3:
		PPUREG.OAMADDR = Value;
		break;
	case 4:
		OAM[PPUREG.OAMADDR++] = Value;
		break;
	case 5:
		if (OffsetToggle)
		{
			PPUSCROLL.YFine = Value & 7;
			PPUSCROLL.YCoarse = Value >> 3;
		}
		else PPUSCROLL.XScroll = Value;
		OffsetToggle = !OffsetToggle;
		break;
	case 6:
		if (OffsetToggle)
		{
			PPUSCROLL.VAddrLo = Value;
			PPUADDR.raw = (uint32)PPUSCROLL.raw;
		}
		else PPUSCROLL.VAddrHi = Value & 0x3F;
		OffsetToggle = !OffsetToggle;
		break;
	case 7:
		{
			RetValue = ReadBuffer;
			uint8& temp = MemoryMap(PPUADDR.raw);
			RetValue = temp = Value;
			RefreshOpenBus(RetValue);
			PPUADDR.raw = PPUADDR.raw + (PPUREG.AddressIncrement ? 32 : 1);
		}
		break;
	default:
		break;
	}
	return RetValue;
}

uint8& PPU::MemoryMap(int32 raw )
{
	raw &= 0x3FFF;
	if (raw >= 0x3F00)
	{
		if (raw%4 == 0) raw &= 0x0F;
		return Palette[raw & 0x1F];
	}
	else if (raw < 0x2000)
	{
		uint32 i = (raw / game->VROMGranularity) % game->VROMPages;
		uint32 j = raw % game->VROMGranularity;
		return game->VBanks[i][j];
	}
	else return game->NameTable[(raw>>10)&3][raw&0x3FF];
}

uint8 PPU::RefreshOpenBus( uint8 RetValue )
{
	return OpenBusDecayTimer = 77777, OpenBus = RetValue;
}

void PPU::SetGame( NESGame* g )
{
	game = g;
}

void PPU::SetIO( IO* i )
{
	io = i;
}

bool PPU::GetNMI()
{
	if (NMIWasSet)
	{
		NMIWasSet = false;
		NMIWasRead = true;
		return true;
	}
	return false;
}

void PPU::RenderTick()
{
	bool TileDecodeMode = (0x10FFFF & (1u << (x/16))) != 0;

	uint32 p;
	switch(x % 8)
	{
	case 2:
		IOAddress = 0x23C0 + 0x400*PPUADDR.NameTableAddress + 8*(PPUADDR.YCoarse/4) + (PPUADDR.XCoarse/4);
		if (TileDecodeMode) break;
	case 0:
		// Point to nametable
		IOAddress = 0x2000 + (PPUADDR.raw & 0xFFF);

		// Reset sprite data
		if (x == 0)
		{
			SpriteInPosition = SpriteOutPosition = 0;
			if (PPUREG.ShowSP) PPUREG.OAMADDR = 0;
		}
		if (!PPUREG.ShowBG) break;

		// Reset scrolling (vertical once, horizontal each scanline)
		if (x == 304 && Scanline == -1) PPUADDR.raw = (uint32)PPUSCROLL.raw;
		if (x == 256)
		{
			PPUADDR.XCoarse = (uint32)PPUSCROLL.XCoarse;
			PPUADDR.HorizNameTableIndex = (uint32)PPUSCROLL.HorizNameTableIndex;
			SpriteRenderPosition = 0;
		}
		break;
	case 1:
		if (x == 337 && Scanline == -1 && EvenOddToggle && PPUREG.ShowBG) ScanlineEnd = 340;

		// Access name table
		PatternAddress = 0x1000*PPUREG.BackgroundPatternTableAddress + 16*MemoryMap(IOAddress) + PPUADDR.YFine;
		if (!TileDecodeMode) break;

		// Push tile into shift registers
		BGShiftPattern = (BGShiftPattern >> 16) + 0x00010000 * TilePattern;
		BGShiftAttribute = (BGShiftAttribute >> 16) + 0x55550000 * TileAttribute;
		break;
	case 3:
		if (TileDecodeMode)
		{
			TileAttribute = (MemoryMap(IOAddress) >> ((PPUADDR.XCoarse&2) + 2*(PPUADDR.YCoarse&2))) & 3;

			// Go to next tile horizontally
			if (!++PPUADDR.XCoarse) PPUADDR.HorizNameTableIndex = 1 - PPUADDR.HorizNameTableIndex;

			// Do the same but vertically if at the edge of the screen
			if (x == 251 && !++PPUADDR.YFine && ++PPUADDR.YCoarse == 30)
			{
				PPUADDR.YCoarse = 0;
				PPUADDR.VertNameTableIndex = 1 - PPUADDR.VertNameTableIndex;
			}
		}
		else if (SpriteRenderPosition < SpriteOutPosition)
		{
			// Copy sprite from OAM2 to OAM3
			auto& o = OAM3[SpriteRenderPosition];
			memcpy(&o, &OAM2[SpriteRenderPosition], sizeof(o));
			uint32 y = Scanline - o.y;
			if (o.Attribute & 0x80) y ^= (PPUREG.SpriteSize ? 15 : 7);
			PatternAddress = 0x1000*(PPUREG.SpriteSize ? (o.Index&0x01) : PPUREG.SpritePatternTableAddress);
			PatternAddress += 0x10*(PPUREG.SpriteSize ? (o.Index&0xFE) : (o.Index&0xFF));
			PatternAddress += (y&7) + (y&8)*2;
		}
		break;
	case 5:
		TilePattern = MemoryMap(PatternAddress | 0);
		break;
	case 7:
		p = TilePattern | (MemoryMap(PatternAddress | 8) << 8);
		p = (p&0xF00F) | ((p&0x0F00)>>4) | ((p&0x00F0)<<4);
		p = (p&0xC3C3) | ((p&0x3030)>>2) | ((p&0x0C0C)<<2);
		p = (p&0x9999) | ((p&0x4444)>>1) | ((p&0x2222)<<1);
		TilePattern = p;
		if (!TileDecodeMode && SpriteRenderPosition < SpriteOutPosition) OAM3[SpriteRenderPosition++].Pattern = TilePattern;
		break;
	default:
		break;
	}

	// Find which sprites are visible on next scanline (TODO: implement crazy 9-sprite malfunction)
	switch (x>=64 && x<256 && x%2 ? (PPUREG.OAMADDR++ & 3) : 4)
	{
	default:
		// Access OAM (object attribute memory)
		SpriteTemp = OAM[PPUREG.OAMADDR];
		break;
	case 0:
		if (SpriteInPosition >= 64)
		{
			PPUREG.OAMADDR=0;
			break;
		}
		++SpriteInPosition; // next sprite
		if (SpriteOutPosition < 8) OAM2[SpriteOutPosition].y = SpriteTemp;
		if (SpriteOutPosition < 8) OAM2[SpriteOutPosition].SpriteIndex = PPUREG.OAMIndex;
		{
			int32 y1 = SpriteTemp, y2 = SpriteTemp + (PPUREG.SpriteSize?16:8);
			if(!( Scanline >= y1 && Scanline < y2 ))
			{
				PPUREG.OAMADDR = SpriteInPosition != 2 ? PPUREG.OAMADDR+3 : 8;
			}
		}
		break;
	case 1:
		if (SpriteOutPosition < 8) OAM2[SpriteOutPosition].Index = SpriteTemp;
		break;
	case 2:
		if (SpriteOutPosition < 8) OAM2[SpriteOutPosition].Attribute = SpriteTemp;
		break;
	case 3:
		if (SpriteOutPosition < 8) OAM2[SpriteOutPosition].x = SpriteTemp;

		if (SpriteOutPosition < 8) ++SpriteOutPosition;
		else PPUREG.SpriteOverflow = true;

		if (SpriteInPosition == 2) PPUREG.OAMADDR = 8;
		break;
	}
}

void PPU::RenderPixel()
{
	bool Edge = uint8(x+8) < 16;
	bool ShowBG = PPUREG.ShowBG && (!Edge || PPUREG.ShowBG8);
	bool ShowSP = PPUREG.ShowSP && (!Edge || PPUREG.ShowSP8);

	// Render the background
	uint32 Effects = PPUSCROLL.XFine, XPos = 15 - (( (x&7) + Effects + 8*!!(x&7) ) & 15);

	uint32 Pixel = 0, Attribute = 0;
	if (ShowBG) // Pick a pixel from the shift registers
	{
		Pixel = (BGShiftPattern >> (XPos*2)) & 3;
		Attribute = (BGShiftAttribute >> (XPos*2)) & (Pixel ? 3 : 0);
	}
	else if ((PPUADDR.raw & 0x3F00) == 0x3F00 && !PPUREG.ShowBG && !PPUREG.ShowSP)
	{
		Pixel = PPUADDR.raw;
	}

	// Overlay sprites
	if (ShowSP)
	{
		for (uint32 SpriteNumber=0; SpriteNumber < SpriteRenderPosition; ++SpriteNumber)
		{
			auto& sprite = OAM3[SpriteNumber];

			// Check if the sprite is in range horizontally
			uint32 XDiff = x - sprite.x;
			if (XDiff >= 8) continue; // Catches negative values too

			// Determine which pixel to display
			if (!(sprite.Attribute & 0x40)) XDiff = 7 - XDiff;
			uint8 SpritePixel = (sprite.Pattern >> (XDiff*2)) & 3;
			if (!SpritePixel) continue;

			// Register sprite-0 hit if needed
			if (x < 255 && Pixel && sprite.SpriteIndex == 0) PPUREG.Sprite0Hit = true;

			// Render the pixel, unless behind-background placement is wanted
			if (!(sprite.Attribute & 0x20) || !Pixel)
			{
				Attribute = (sprite.Attribute & 3) + 4;
				Pixel = SpritePixel;
			}
			
			// Only process the first non-transparent sprite pixel
			break;
		}
	}
	Pixel = Palette[ (Attribute*4 + Pixel) & 0x1F ] & (PPUREG.Grayscale ? 0x30 : 0x3F);
	uint32 Intensify = ((PPUREG.IntensifyReds << 2) + (PPUREG.IntensifyGreens << 1) + PPUREG.IntensifyBlues) << 6;
	io->SetPixel(x, Scanline, Pixel | Intensify, CycleCounter);
}

void PPU::Init()
{
	PPUREG.raw = 0;
	PPUSCROLL.data = 0;
	PPUADDR.data = 0;

	for(int32 i=0; i<256; i++) OAM[i]=0;
	for (int32 i=0; i<32; i++) Palette[i] = 0;
}

void PPU::Dump()
{
	std::cout << "PPU Data" << std::endl;
	std::cout << "x = " << std::hex << x << "; Scanline = " << Scanline << "; PatternAddress = " << PatternAddress << "; SprRPos = " << SpriteRenderPosition << std::endl;
	std::cout << "Registers: " << std::bitset<32>(PPUREG.raw) << std::endl;
	std::cout << "OAM:" << std::endl;
	for (int i=0; i<256; i++)
	{
		PrintHex(OAM[i]);
		if (i%40 == 0 && i>0) std::cout << std::endl;
	}
	std::cout << std::endl;
	std::cout << "OAM2 and OAM3:" << std::endl;
	for (int32 i=0; i<8; i++)
	{
		std::cout << (int)OAM2[i].SpriteIndex << "\t" << (int)OAM3[i].SpriteIndex << std::endl;
		std::cout << (int)OAM2[i].x << "\t" << (int)OAM3[i].x << std::endl;
		std::cout << (int)OAM2[i].y << "\t" << (int)OAM3[i].y << std::endl;
		std::cout << (int)OAM2[i].Index << "\t" << (int)OAM3[i].Index << std::endl;
		std::cout << (int)OAM2[i].Attribute << "\t" << (int)OAM3[i].Attribute << std::endl;
		std::cout << OAM2[i].Pattern << "\t" << OAM3[i].Pattern << std::endl;
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

