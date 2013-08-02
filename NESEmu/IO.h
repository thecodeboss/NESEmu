#ifndef IO_h__
#define IO_h__

#include "Global.h"

class IO
{
	SDL_Surface *screen;
	uint32 Palette[3][64][512];
	uint32 PreviousPixel;

	float GammaFix(float f);
	int32 Clamp(int32 i);

public:
	IO();
	bool Init();
	void FlushScanline(uint32 y);
	void SetPixel(uint32 x, uint32 y, uint32 pixel, int32 offset);
};

#endif // IO_h__
