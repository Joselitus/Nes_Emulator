#ifndef CIM_H
#define CIM_H

#include <SDL2/SDL.h>
#include "BUS.h"

class CIM {
private:
	BUS * bus;
public:
	CIM(BUS * bus);
	~CIM();
	void handlePress(SDL_Event e);
	void handleRelease(SDL_Event e);
	void handleJoyPress(SDL_Event e);
	void handleJoyRelease(SDL_Event e);
};

#endif