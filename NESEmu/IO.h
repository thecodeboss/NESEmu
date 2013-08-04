#ifndef IO_h__
#define IO_h__

#include "Global.h"

class IO
{
	SDL_Surface *screen;
	SDL_Event event;
	uint32 Palette[3][64][512];
	uint32 PreviousPixel, FrameCount;
	bool quit;
	bool ScanlineFlushed;
	bool FrameDump;
	float GammaFix(float f);
	uint32 Clamp(float i);

public:
	IO();
	bool Init();
	void FlushScanline(uint32 y);
	void SetPixel(uint32 x, uint32 y, uint32 pixel, int32 offset);
	void InitPalette();
	bool Poll();
	void SetFrameDump( bool set );
};

#endif // IO_h__
