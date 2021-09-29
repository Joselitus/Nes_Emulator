#include "CIM.h"

CIM::CIM(BUS * bus) {
	this->bus = bus;
	if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
		std::cout << "couldn't initialize controller handler" << std::endl;

    //Check for joysticks
    num_controllers = SDL_NumJoysticks();
    if (num_controllers > 0) {
        //Load joystick
        controller1 = SDL_GameControllerOpen( 0 );
        if( controller1 == NULL ) {
            printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
        }
    }

}

void CIM::newController() {
	switch ( num_controllers ) {
		case 0:
			controller1 = SDL_GameControllerOpen( 0 );
	        if( controller1 == NULL ) {
	            printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
	        }
		break;
	}
}

void CIM::closeController() {
	SDL_GameControllerClose(controller1);
}

void CIM::handleJoyAxis(SDL_Event e) {
	if (!e.jaxis.axis) {
		if (e.jaxis.value > 25000)
			bus->pressRt();
		else if (e.jaxis.value < -25000)
			bus->pressLt();
		else {
			bus->releaseLt();
			bus->releaseRt();
		}
	}
	else {
		if (e.jaxis.value > 25000)
			bus->pressDn();
		else if (e.jaxis.value < -25000)
			bus->pressUp();
		else {
			bus->releaseUp();
			bus->releaseDn();
		}
	}
}


void CIM::handlePress(SDL_Event e) {
	switch(e.key.keysym.scancode) {

		case SDL_SCANCODE_W:
			bus->pressUp();
		break;
		case SDL_SCANCODE_A:
			bus->pressLt();
		break;
		case SDL_SCANCODE_S:
			bus->pressDn();
		break;
		case SDL_SCANCODE_D:
			bus->pressRt();
		break;
		case SDL_SCANCODE_V:
			bus->pressSl();
		break;
		case SDL_SCANCODE_B:
			bus->pressSt();
		break;
		case SDL_SCANCODE_N:
			bus->pressA();
		break;
		case SDL_SCANCODE_M:
			bus->pressB();
		break;
	}
}
void CIM::handleRelease(SDL_Event e) {
	switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_W:
			bus->releaseUp();
		break;
		case SDL_SCANCODE_A:
			bus->releaseLt();
		break;
		case SDL_SCANCODE_S:
			bus->releaseDn();
		break;
		case SDL_SCANCODE_D:
			bus->releaseRt();
		break;
		case SDL_SCANCODE_V:
			bus->releaseSl();
		break;
		case SDL_SCANCODE_B:
			bus->releaseSt();
		break;
		case SDL_SCANCODE_N:
			bus->releaseA();
		break;
		case SDL_SCANCODE_M:
			bus->releaseB();
		break;
	}
}

void CIM::handleJoyPress(SDL_Event e) {
	//std::cout << (uint16_t)e.jbutton.button << std::endl;
	switch(e.jbutton.button) {

		case 13:
			bus->pressUp();
		break;
		case 15:
			bus->pressLt();
		break;
		case 14:
			bus->pressDn();
		break;
		case 16:
			bus->pressRt();
		break;
		case 8:
			bus->pressSl();
		break;
		case 9:
			bus->pressSt();
		break;
		case 1:
			bus->pressA();
		break;
		case 0:
			bus->pressB();
		break;
	}
}
void CIM::handleJoyRelease(SDL_Event e) {
	switch (e.jbutton.button) {
		case 13:
			bus->releaseUp();
		break;
		case 15:
			bus->releaseLt();
		break;
		case 14:
			bus->releaseDn();
		break;
		case 16:
			bus->releaseRt();
		break;
		case 8:
			bus->releaseSl();
		break;
		case 9:
			bus->releaseSt();
		break;
		case 1:
			bus->releaseA();
		break;
		case 0:
			bus->releaseB();
		break;
	}
}