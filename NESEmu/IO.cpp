#include "IO.h"
using namespace std;

IO::IO()
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

	return true;
}
