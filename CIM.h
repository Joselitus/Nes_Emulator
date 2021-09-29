#ifndef CIM_H
#define CIM_H

#include <SDL2/SDL.h>
#include "BUS.h"

class CIM {
private:
	BUS * bus;
	int num_controllers;
	SDL_GameController * controller1;
public:
	CIM(BUS * bus);
	~CIM();
	void handlePress(SDL_Event e);
	void handleRelease(SDL_Event e);
	void handleJoyPress(SDL_Event e);
	void handleJoyRelease(SDL_Event e);
	void handleJoyAxis(SDL_Event e);
	void newController();
	void closeController();
};

#endif