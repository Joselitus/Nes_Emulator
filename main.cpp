#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

#include "CPU.h"
#include "PPU.h"
#include "ROM.h"
#include "CIM.h"

#define DW 256
#define DH 240

#define SETTINGS "settings.st"

#define PS "PS"
#define DEFAULT_PS 4

#define VOLUME "VOLUME"
#define DEFAULT_VOLUME 1000

// Giving samples for the audio stream
void audio_callback(void *userdata, Uint8 *_stream, int _length)
{
    Sint16 *stream = (Sint16*) _stream;
    int length = _length / 2;
    ((APU*)userdata)->getSamples(stream, length);
}

int main(int argc, char const *argv[])
{
	if ( argc < 2 ) {
		std::cerr << "A valid rom file must be specified as the first parameter" << std::endl;
		exit( EXIT_FAILURE );
	}

	int pixel_size = DEFAULT_PS;
	int volume = DEFAULT_VOLUME;
	char * token;
	ifstream infile(SETTINGS);
	for( std::string settings_line; getline(infile, settings_line );) {
		token = strtok((char*)settings_line.c_str(), ":");
		if (!token) continue;
		if (!strcmp(token, PS)) {
			pixel_size = atoi(strtok(NULL, ":"));
		}
		if (!strcmp(token, VOLUME)) {
			volume = atoi(strtok(NULL, ":"));
		}
	}


	ROM * rom = new ROM();
	if ( !rom->load(std::string(argv[1])) )
		exit(EXIT_FAILURE);
	Display * dis = new Display(DW, DH, pixel_size);
	PPU * ppu = new PPU(dis);
	APU * apu = new APU(volume);
	BUS * bus = new BUS(ppu, apu, rom);
	CPU * cpu = new CPU(bus);
	CIM * cim = new CIM(bus);

	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec desiredSpec;

	long wavetime = 0;

    desiredSpec.freq = FREQUENCY;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 2048;
    desiredSpec.callback = audio_callback;
    desiredSpec.userdata = apu;

    SDL_AudioSpec obtainedSpec;
    SDL_OpenAudio(&desiredSpec, &obtainedSpec);
    SDL_PauseAudio(0);

	SDL_Event e;
	int end = 0;
	int cyc = 0;
	while ( !end )	{
		cyc = (cyc + 1)%6;
		if ( !(cyc%3) )
			cpu->tick();
		apu->tick();
		if ( ppu->tick() ) {
			while( SDL_PollEvent( &e ) > 0 ) {
				if( e.type == SDL_QUIT ) end = 1;

				if ( e.type == SDL_KEYDOWN ) {
					switch (e.key.keysym.scancode) {
					
					case (SDL_SCANCODE_R):
						cpu->reset();
						ppu->reset();
						rom->reset();
						break;
					case (SDL_SCANCODE_V):
						apu->amplitude += 100;
						break;
					case (SDL_SCANCODE_B):
						apu->amplitude -= 100;
						if(apu->amplitude < 0) apu->amplitude = 0;
						break;
					default:
						cim->handlePress(e);
					}
				}
				if ( e.type == SDL_KEYUP ) {
					cim->handleRelease(e);
				}
				if ( e.type == SDL_JOYBUTTONDOWN ) {
					cim->handleJoyPress(e);
				}
				if ( e.type == SDL_JOYBUTTONUP) {
					cim->handleJoyRelease(e);
				}
				if ( e.type == SDL_JOYAXISMOTION ) {
					cim->handleJoyAxis(e);
				}
				if ( e.type == SDL_JOYDEVICEADDED ) {
					cim->newController();
				}
				if ( e.type == SDL_JOYDEVICEREMOVED ) {
					cim->closeController();
				}

			}
		}
	}
	rom->save();
	return 0;
}
