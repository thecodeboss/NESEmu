#ifndef IO_h__
#define IO_h__

#include "Global.h"

class IO
{
	SDL_Surface *screen;
public:
	IO();
	bool Init();
};

#endif // IO_h__
