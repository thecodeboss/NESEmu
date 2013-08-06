#ifndef IO_h__
#define IO_h__

#include "Global.h"
#include "Xbox360Controller.h"
#include "resampler.h"
#include <sstream>
#include <map>
#include <queue>

class IO
{
	SDL_Surface *screen;
	SDL_Event event;
	SDL_Joystick *joystick;
	std::map<uint8, bool> HatStatus;
	uint32 Palette[64];
	uint32 NTSCPalette[3][64][512];
	uint32 PreviousPixel, FrameCount;
	SDL_AudioSpec *desired, *obtained;
	std::queue<uint8> AudioStream;
	Resampler* resampler;
	bool quit;
	bool ScanlineFlushed;
	bool FrameDump;
	bool NTSCMode;
	int32 VerticalScale;
	int32 HorizontalScale;
	int32 CurrentJoystick[2];
	int32 NextJoystick[2];
	int32 JoystickPosition[2];
	float FPS;
	uint32 StartTime;
	bool bXBOX360Controller;
	bool AudioRunning;
	bool ReadInProgress;
	uint8 AudioStreamBuffer[2048];
	uint8 tempstream[4096];
	float GammaFix(float f);
	uint32 Clamp(float i);

public:
	IO();
	bool Init(int32 width = 256, int32 height = 240);
	void FlushScanline(uint32 y);
	void SetPixel(uint32 x, uint32 y, uint32 pixel, int32 offset = 0);
	void InitPalettes();
	bool Poll();
	void SetFrameDump( bool set );
	void SetNTSCMode(bool b);
	void StrobeJoystick(uint32 Value);
	uint8 ReadJoystick(uint32 Index);
	void SetFPS( float v );
	void StartClock();
	void WaitForClock();
	void WriteAudioStream(uint8 in);
	void ReadAudioMix(void *unused, uint8 *stream, int32 len);
	void HandleInput();
};

void NonMemberReadAudioMix( void *io, uint8 *stream, int32 len );

#endif // IO_h__
