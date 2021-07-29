#include "CIM.h"

CIM::CIM(BUS * bus) {
	this->bus = bus;
	if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
		std::cout << "couldn't initialize controller handler" << std::endl;
	//Set texture filtering to linear
    if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
    {
        printf( "Warning: Linear texture filtering not enabled!" );
    }

    //Check for joysticks
    if( SDL_NumJoysticks() < 1 )
    {
        printf( "Warning: No joysticks connected!\n" );
    }
    else
    {
        //Load joystick
        SDL_Joystick * gGameController = SDL_JoystickOpen( 0 );
        if( gGameController == NULL )
        {
            printf( "Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError() );
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