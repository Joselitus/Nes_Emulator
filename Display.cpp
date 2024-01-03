#include "Display.h"

Display::Display(int width, int height, int pixel_size) {
	this->width = width;
	this->height = height;
	this->pixel_size = pixel_size;

	// Initialize sdl video
	if( SDL_Init( SDL_INIT_VIDEO ) < 0){
		std::cerr << "Could not initialise SDL: " << SDL_GetError << std::endl;
         exit( EXIT_FAILURE );
     }
    window =  SDL_CreateWindow("bingle", 0, 0, width*pixel_size, height*pixel_size, 0);
    SDL_Surface * icon = SDL_LoadBMP_RW(SDL_RWFromMem((void * )iconnes_bmp, 258790), 1);
    SDL_SetWindowIcon(window, icon);
    surface = SDL_GetWindowSurface(window);
}

uint32_t Display::convertColour(uint16_t nescolour) {
	uint32_t blue = ((uint32_t)nescolour & 0x000F) << 5;
	uint32_t green = ((uint32_t)nescolour & 0x00F0) << 9;
	uint32_t red = ((uint32_t)nescolour & 0x0F00) << 13;
	return blue + green + red;
}

void Display::refersh() {
	SDL_UpdateWindowSurface(window);
}

void Display::setPixel(int x, int y, uint16_t nescolour) {
	uint32_t colour = convertColour(nescolour);
	if (x < 0 || y < 0) return;
	if (x >= width || y >= height) return;
	uint32_t * screen = (uint32_t*)surface->pixels;
	#pragma omp parallel for
	for (int i = 0; i < pixel_size; i++) {
		for (int j = 0; j < pixel_size; j++) {
			screen[(pixel_size*y+i)*width*pixel_size + pixel_size*x+j] = colour;
		}
	}
}

void Display::setRectangle(int x, int y, int w, int h, uint16_t nescolour) {
	for (int i = 0; i < w; i++)
		for (int j = 0; j < h; j++)
			setPixel(x + i, y + j, nescolour);
}