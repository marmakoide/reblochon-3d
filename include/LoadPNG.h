#ifndef REBLOCHON_LOAD_PNG_H
#define REBLOCHON_LOAD_PNG_H

#include <SDL.h>



namespace reb {
	/*
	 * Load a PNG image, accepting only 8 bits indexed colors images
	 */

	SDL_Surface*
	load_png(const char* path);
} // namespace reb



#endif // REBLOCHON_LOAD_PNG_H
