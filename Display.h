#ifndef DISPLAY_H
#define DISPLAY_H

#include <cstdint>
#include <iostream>
#include <SDL2/SDL.h>

#include "Icon.h"

class Display
{
private:
	int width;
	int height;
	int pixel_size;

	SDL_Surface * surface;
	SDL_Window * window;

	uint32_t convertColour(uint16_t nescolour);
public:
	Display(int width, int height, int pixel_size);
	~Display();
	void refersh();
	void setPixel(int x, int y, uint16_t colour);
	void setRectangle(int x, int y, int w, int h, uint16_t nescolour);
	
};

#endif