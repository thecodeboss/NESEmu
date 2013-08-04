#include "IO.h"
#include <sstream>
using namespace std;

IO::IO() : PreviousPixel(~0u), FrameCount(0), FrameDump(false)
{

}

bool IO::Init()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode(256, 240, 32, 0);
	SDL_WM_SetCaption("The Code Boss's NES Emulator", NULL );

	if (screen == NULL)
	{
		cout << "Failed to set SDL video mode" << endl;
		return false;
	}

	InitPalette();

	return true;
}

void IO::FlushScanline( uint32 y )
{
	if (y == 239)
	{
		SDL_Flip(screen);
		ScanlineFlushed = true;
		if (FrameDump)
		{
			std::stringstream s;
			s << "..\\frames\\frame";
			if (FrameCount < 10) s << "000";
			else if (FrameCount < 100) s << "00";
			else if (FrameCount < 1000) s << "0";
			s << FrameCount << ".bmp";
			std::string Filename;
			s >> Filename;
			std::cout << Filename << std::endl;
			SDL_SaveBMP( screen, Filename.c_str());
			FrameCount++;
		}
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

uint32 IO::Clamp( float i )
{
	return i > 255 ? 255 : static_cast<uint32>(i);
}

void IO::InitPalette()
{
	for (int32 i=0; i<3; i++) for (int32 j=0; j<64; j++) for (int32 k=0; k<512; k++)
		Palette[i][j][k] = 0;

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

	SetPixel(0,0,0,0);
}

bool IO::Poll()
{
	if (ScanlineFlushed)
	{
		SDL_PollEvent( &event );
		if( event.type == SDL_QUIT ) return false;
		ScanlineFlushed = false;
	}
	return true;
}

void IO::SetFrameDump( bool set )
{
	FrameDump = true;
}
