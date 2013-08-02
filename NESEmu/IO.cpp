#include "IO.h"
using namespace std;

IO::IO() : PreviousPixel(~0u)
{

}

bool IO::Init()
{
	if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 ) {
		cout << "Failed to init SDL" << endl;
		return false;
	}

	screen = SDL_SetVideoMode(256, 250, 16, SDL_SWSURFACE);
	if (screen == NULL)
	{
		cout << "Failed to set SDL video mode" << endl;
		return false;
	}

	// Note: the following snippet is largely taken from Bisqwit
	for(int32 o=0; o<3; ++o)
		for(int32 u=0; u<3; ++u)
			for(int32 p0=0; p0<512; ++p0)
				for(int32 p1=0; p1<64; ++p1)
				{
					// Calculate the luma and chroma by emulating the relevant circuits:
					auto s = "\372\273\32\305\35\311I\330D\357}\13D!}N";
					int32 y=0, i=0, q=0;
					for(int32 p=0; p<12; ++p) // 12 samples of NTSC signal constitute a color.
					{
						// Sample either the previous or the current pixel.
						int32 r = (p+o*4)%12, pixel = r < 8-u*2 ? p0 : p1; // Use pixel=p0 to disable artifacts.
						// Decode the color index.
						int32 c = pixel%16, l = c<0xE ? pixel/4 & 12 : 4, e=p0/64;
						// NES NTSC modulator (square wave between up to four voltage levels):
						int32 b = 40 + s[(c > 12*((c+8+p)%12 < 6)) + 2*!(0451326 >> p/2*3 & e) + l];
						// Ideal TV NTSC demodulator:
						y += b;
						i += b * int32(std::cos(M_PI * p / 6) * 5909);
						q += b * int32(std::sin(M_PI * p / 6) * 5909);
					}
					// Store color at subpixel precision
					float Y = y/1980.f, I = i/9e6f, Q = q/9e6f; // Convert from int to float
					if(u==2) Palette[o][p1][p0] += 0x10000*Clamp(255 * GammaFix(Y + I* 0.947f + Q* 0.624f));
					if(u==1) Palette[o][p1][p0] += 0x00100*Clamp(255 * GammaFix(Y + I*-0.275f + Q*-0.636f));
					if(u==0) Palette[o][p1][p0] += 0x00001*Clamp(255 * GammaFix(Y + I*-1.109f + Q* 1.709f));
				}

	return true;
}

void IO::FlushScanline( uint32 y )
{
	if (y == 239)
	{
		SDL_Flip(screen);
	}
}

void IO::SetPixel( uint32 x, uint32 y, uint32 pixel, int32 offset )
{
	((uint32*) screen->pixels) [y * 256 + x] = Palette[offset][PreviousPixel%64][pixel];
	PreviousPixel = pixel;
}

float IO::GammaFix( float f )
{
	return f <= 0.f ? 0.f : std::pow(f, 2.2f / 1.8f);
}

int32 IO::Clamp( int32 i )
{
	return i > 255 ? 255 : i;
}
