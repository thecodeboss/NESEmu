#include "IO.h"
#include "Palette.h"
#include <windows.h> // I hate to include this, but I want access to CreateDirectory()
using namespace std;

IO::IO() : PreviousPixel(~0u), FrameCount(0), FrameDump(false), AudioDump(false), NTSCMode(false), StartTime(0), joystick(NULL), bXBOX360Controller(false), AudioRunning(false), ReadInProgress(false), ShouldSaveGame(false)
{
	CurrentJoystick[0] = 0;
	CurrentJoystick[1] = 0;
	NextJoystick[0] = 0;
	NextJoystick[1] = 0;
	JoystickPosition[0] = 0;
	JoystickPosition[1] = 0;
	for (int32 i=0; i<38182; i++) AudioStreamBuffer[i] = 0;
}

IO::~IO()
{
	// There doesn't seem to be
	SDL_Delay(100); // Sleep 1/10 second

	// Before deleting IO, ensure the audio thread finishes what it's
	// doing or it'll be eating bad memory.
	SDL_CloseAudio();

	delete desired;
	delete obtained;

	// Also SDL leaks if we don't manually 'quit'
	SDL_Quit();

	delete resampler;
}

bool IO::Init(int32 hScale, int32 vScale)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
	screen = SDL_SetVideoMode(hScale*256, vScale*240, 32, 0);
	SDL_WM_SetCaption("The Code Boss's NES Emulator", NULL );

	if (screen == NULL)
	{
		cout << "Failed to set SDL video mode" << endl;
		return false;
	}

	HorizontalScale = hScale;
	VerticalScale = vScale;

	desired = (SDL_AudioSpec *) malloc(sizeof(SDL_AudioSpec));
	obtained = (SDL_AudioSpec *) malloc(sizeof(SDL_AudioSpec));
	desired->freq = 48000;
	desired->format = AUDIO_S16;
	desired->samples = 1024;
	desired->callback = NonMemberReadAudioMix;
	desired->userdata = this;
	desired->channels = 1;

	if ( SDL_OpenAudio(desired, obtained) < 0 ) {
		cout << "AudioMixer, Unable to open audio: " << SDL_GetError() << endl;
		return false;
	}

	resampler = new Resampler();
	resampler->setup(1789800, 48000, 1, 16);

	InitPalettes();

	if (SDL_NumJoysticks())
	{
		cout << SDL_NumJoysticks() << " joysticks were found.\n\nThe names of the joysticks are:\n";

		for(int32 i=0; i < SDL_NumJoysticks(); i++ )
		{
			cout << SDL_JoystickName(i) << endl;
			// @TODO: Should definitely make this a better string search, but I'm lazy for now.
			if ((strcmp(SDL_JoystickName(i), "Controller (XBOX 360 For Windows)") == 0
				|| strcmp(SDL_JoystickName(i), "Controller (Xbox 360 Wireless Receiver for Windows)") == 0) && !bXBOX360Controller)
			{
				SDL_JoystickEventState(SDL_ENABLE);
				joystick = SDL_JoystickOpen(0);
				cout << "Using Joystick 0 for Player 1, Keyboard for Player 2" << endl;
				bXBOX360Controller = true;

				// Initialize XBOX 360 Controller Hat values
				HatStatus[SDL_HAT_UP] = false;
				HatStatus[SDL_HAT_DOWN] = false;
				HatStatus[SDL_HAT_LEFT] = false;
				HatStatus[SDL_HAT_RIGHT] = false;
			}
		}
	}

	return true;
}

void IO::FlushScanline( uint32 y )
{
	if (y == 239)
	{
		FrameCount++;

		if (!AudioRunning)
		{
			SDL_PauseAudio(0);
			AudioRunning = true;
		}

		WaitForClock();
		ScanlineFlushed = true;
		if (FrameDump)
		{
			std::stringstream s;
			s << "frames\\frame";
			if (FrameCount < 10) s << "0000";
			else if (FrameCount < 100) s << "000";
			else if (FrameCount < 1000) s << "00";
			else if (FrameCount < 10000) s << "0";
			s << FrameCount << ".bmp";
			std::string Filename;
			s >> Filename;
			SDL_SaveBMP( screen, Filename.c_str());
		}
	}
}

void IO::SetPixel( uint32 x, uint32 y, uint32 pixel, int32 offset )
{
	int32 yv = y*VerticalScale, xh = x*HorizontalScale, z = 256*HorizontalScale;
	if (NTSCMode)
	{
		for (int32 i=0; i<VerticalScale; ++i) for (int32 j=0; j<HorizontalScale; ++j)
			((uint32*) screen->pixels) [(yv+i) * z + xh + j] = NTSCPalette[offset][PreviousPixel%64][pixel];
		PreviousPixel = pixel;
	}
	else
	{
		for (int32 i=0; i<VerticalScale; ++i) for (int32 j=0; j<HorizontalScale; ++j)
			((uint32*) screen->pixels) [(yv+i) * z + xh + j] = Palette[pixel & 0x3F];
	}
}

float IO::GammaFix( float f )
{
	return f <= 0.f ? 0.f : std::pow(f, 2.2f / 1.8f);
}

uint32 IO::Clamp( float i )
{
	return i > 255 ? 255 : static_cast<uint32>(i);
}

void IO::InitPalettes()
{
	for (int32 i=0; i<3; i++) for (int32 j=0; j<64; j++) for (int32 k=0; k<512; k++)
		NTSCPalette[i][j][k] = 0;

	// Set the 64 colours of the palette, increasing their intensity by 4x
	// (It seems to look nice like this)
	for (int32 i=0; i<64; i++)
	{
		Palette[i] = Palettes::default[i];
		uint32 pr = (Palette[i] & 0xFF0000) >> 16, pg = (Palette[i] & 0x00FF00) >> 8, pb = Palette[i] & 0x0000FF;
		Palette[i] = 0x10000*Clamp(pr*4.f) + 0x00100*Clamp(pg*4.f) + 0x00001*Clamp(pb*4.f);
	}

	// Note: the following nested loop is largely taken from Bisqwit and the NES Dev Wiki
	// http://wiki.nesdev.com/w/index.php/NTSC_video
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
					if(u==2) NTSCPalette[o][p1][p0] += 0x10000*Clamp(255 * GammaFix(Y + I* 0.947f + Q* 0.624f));
					if(u==1) NTSCPalette[o][p1][p0] += 0x00100*Clamp(255 * GammaFix(Y + I*-0.275f + Q*-0.636f));
					if(u==0) NTSCPalette[o][p1][p0] += 0x00001*Clamp(255 * GammaFix(Y + I*-1.109f + Q* 1.709f));
				}

	SetPixel(0,0,0,0);
}

bool IO::Poll()
{
	if (ScanlineFlushed)
	{
		SDL_PollEvent( &event );

		HandleInput();
		
		if( event.type == SDL_QUIT ) return false;
		ScanlineFlushed = false;

	}
	return true;
}

void IO::SetFrameDump( bool set )
{
	if (CreateDirectory(L"frames", NULL) ||
		ERROR_ALREADY_EXISTS == GetLastError())
	{
		FrameDump = true;
		cout << "Framedump successfully started." << endl;
	}
	else
	{
		cout << "Framedump failed to start: directory creation failed." << endl;
	}
}

void IO::SetAudioDump( bool dump )
{
	AudioDump = true;
}

void IO::SetNTSCMode( bool b )
{
	NTSCMode = b;
}

void IO::StrobeJoystick(uint32 Value)
{
	if (Value)
	{
		CurrentJoystick[0] = NextJoystick[0];
		CurrentJoystick[1] = NextJoystick[1];
		JoystickPosition[0] = 0;
		JoystickPosition[1] = 0;
	}
}

uint8 IO::ReadJoystick( uint32 Index )
{
	static const uint8 masks[8] = {0x20,0x10,0x40,0x80,0x04,0x08,0x02,0x01};
	return ((CurrentJoystick[Index] & masks[JoystickPosition[Index]++ & 7]) ? 1 : 0);
}

void IO::SetFPS( float v )
{
	FPS = v;
}

void IO::StartClock()
{
	StartTime = SDL_GetTicks();
}

void IO::WaitForClock()
{
	bool AtLeastOnce = false;
	while (true)
	{
		float msRunningTime = (float)(SDL_GetTicks() - StartTime);
		float ShouldBeOnFrame = FPS*msRunningTime/1000.f;
		float AheadByFrames = FrameCount - ShouldBeOnFrame;
		if (AheadByFrames >= 1.0f || !AtLeastOnce)
		{
			SDL_Flip(screen);
			AtLeastOnce = true;
		}
		else break;
	}
}

void IO::WriteAudioStream( float in )
{
	static int AudioStreamIndex = 0;
	AudioStreamBuffer[AudioStreamIndex++] = in;
	if (AudioStreamIndex == 38182)
	{
		resampler->inp_count = 38182;
		resampler->out_count = 1024;
		resampler->inp_data = AudioStreamBuffer;
		resampler->out_data = tempstream;
		AudioLock.lock();
		resampler->process();
		AudioLock.unlock();
		AudioStreamIndex = 0;
	}
}

void IO::HandleInput()
{
	static const uint8 masks[8] = {0x20,0x10,0x40,0x80,0x04,0x08,0x02,0x01};
	uint8 KeyboardPlayer = joystick ? 1 : 0;

	// Handle XBOX 360 Controller events
	if (bXBOX360Controller)
	{
		if (event.type == SDL_JOYBUTTONDOWN)
		{
			switch (event.jbutton.button)
			{
				#define t(a,b) case a: NextJoystick[0] |= masks[b]; break;
					t(XBOX360_A, 0)
					t(XBOX360_X, 1)
					t(XBOX360_BACK, 2)
					t(XBOX360_START, 3)
				#undef t
			}
		}
		else if (event.type == SDL_JOYBUTTONUP)
		{
			switch (event.jbutton.button)
			{
				#define t(a,b) case a: NextJoystick[0] &= ~masks[b]; break;
					t(XBOX360_A, 0)
					t(XBOX360_X, 1)
					t(XBOX360_BACK, 2)
					t(XBOX360_START, 3)
				#undef t
				case XBOX360_Y: ShouldSaveGame = true; break;
			}
		}
		else if (event.type == SDL_JOYHATMOTION)
		{
			#define t(s,b) if (event.jhat.value & s) HatStatus[s] = true; else HatStatus[s] = false;
				t(SDL_HAT_UP, 4)
				t(SDL_HAT_DOWN, 5)
				t(SDL_HAT_LEFT, 6)
				t(SDL_HAT_RIGHT, 7)
			#undef t
		}
		#define t(s,b) if (HatStatus[s]) NextJoystick[0] |= masks[b]; else NextJoystick[0] &= ~masks[b];
			t(SDL_HAT_UP, 4)
			t(SDL_HAT_DOWN, 5)
			t(SDL_HAT_LEFT, 6)
			t(SDL_HAT_RIGHT, 7)
		#undef t
	}

	// Handle keyboard events
	if (event.type == SDL_KEYDOWN)
		switch( event.key.keysym.sym )
		{
			#define t(a,b) case a: NextJoystick[KeyboardPlayer] |= masks[b]; break;
				t(SDLK_z, 0)
				t(SDLK_x, 1)
				t(SDLK_LCTRL, 2)t(SDLK_RCTRL, 2)
				t(SDLK_RETURN, 3)
				t(SDLK_UP, 4)
				t(SDLK_DOWN, 5)
				t(SDLK_LEFT, 6)
				t(SDLK_RIGHT, 7)
			#undef t
		}
	else if (event.type == SDL_KEYUP)
		switch( event.key.keysym.sym )
		{
			#define t(a,b) case a: NextJoystick[KeyboardPlayer] &= ~masks[b]; break;
				t(SDLK_z, 0)
				t(SDLK_x, 1)
				t(SDLK_LCTRL, 2)t(SDLK_RCTRL, 2)
				t(SDLK_RETURN, 3)
				t(SDLK_UP, 4)
				t(SDLK_DOWN, 5)
				t(SDLK_LEFT, 6)
				t(SDLK_RIGHT, 7)
			#undef t
			case SDLK_s: ShouldSaveGame = true; break;
		}
}

void IO::ReadAudioMix( void *unused, uint8 *stream, int32 len )
{
	ReadInProgress = true;
	AudioLock.lock();
	for (int i=0; i<len; i++) {
		short temp = static_cast<short>(tempstream[i/2]);
		stream[i++] = uint8(temp & 0xFF);
		stream[i] = uint8(temp / 256);
	}
	AudioLock.unlock();
	ReadInProgress = false;
}

void IO::WriteWAV( const char *outfile )
{
	struct WavHeader
	{
		// RIFF Chunk
		char			ChunkID[4];
		unsigned long	ChunkSize;
		char			Format[4];

		// Format Chunk
		char			FormatChunkID[4];
		unsigned long	FormatChunkSize;
		unsigned short	AudioFormat;
		unsigned short	NumChannels;
		unsigned long	SampleRate;
		unsigned long	ByteRate;
		unsigned short	BlockAlign;
		unsigned short	BitsPerSample;

		// Data Chunk
		char			DataChunkID[4];
		unsigned long	DataChunkSize;
	};

	if (AudioDump)
	{
		unsigned DataSize = static_cast<unsigned>(AudioSamples.size()) * 1024 * 2;
		unsigned FileSize = 36 + DataSize;
		WavHeader temp = { {'R','I','F','F'}, FileSize, {'W','A','V','E'}, {'f','m','t',' '}, 16, 1, 1, 48000, 96000, 2, 16, {'d','a','t','a'}, DataSize};

		std::ofstream stream(outfile, std::ios::binary);
		stream.write((char *)&temp, sizeof(WavHeader));
		for (short * sample : AudioSamples) stream.write((char *)sample, 1024*sizeof(short));
		stream.close();
	}
}

void NonMemberReadAudioMix( void *io, uint8 *stream, int32 len )
{
	static_cast<IO*>(io)->ReadAudioMix(NULL, stream, len);
}
