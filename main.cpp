#include <iostream>
#include<sys/wait.h>
#include <fcntl.h>

using namespace std;

#include "CPU.h"
#include "PPU.h"
#include "ROM.h"
#include "CIM.h"

#define DW 256
#define DH 240
#define PS 4

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

	ROM * rom = new ROM();
	if ( !rom->load(std::string(argv[1])) )
		exit(EXIT_FAILURE);
	Display * dis = new Display(DW, DH, PS);
	PPU * ppu = new PPU(dis);
	APU * apu = new APU();
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
					if (e.key.keysym.scancode == SDL_SCANCODE_R) {
						cpu->reset();
						ppu->reset();
						rom->reset();
					}
					else
						cim->handlePress(e);
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
