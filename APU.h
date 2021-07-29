#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <iostream>
#include <math.h>
#include <functional>
#include <SDL2/SDL.h>

#define AMPLITUDE 5000
#define FREQUENCY 44100
#define PI 3.141592653589793
#define BUFFER_SIZE 14


class LinearCounter
{
public:
	bool control;
	bool reset;
	uint16_t reset_val;
	uint16_t counter;
	LinearCounter() {counter = reset = reset_val = 0;}
	~LinearCounter();
	void tick() {
		if (reset) {
			counter = reset_val;
			reset = false;
		}
		else if (counter)
			counter--;

		if (!control) reset = false;
	}
	
};

class Sweeper
{
public:
	bool enable;
	bool down;
	bool mute;
	bool reset;
	uint8_t shift;
	uint8_t timer;
	uint8_t period;
	uint16_t change;
	Sweeper(){enable = down = mute = reset = shift = timer = period = change = 0;}
	~Sweeper();
	void track(uint16_t reload) {
		if (enable) {
			change = reload >> shift;
			mute = (reload < 8) || (reload > 0x7FF);
		}
	}
	uint16_t tick(uint16_t reload, int channel) {
		if (timer == 0 && enable && shift > 0 && !mute) {
			if (reload >= 8 && change < 0x07FF) {
				if (down)
					reload -= change - channel;
				else
					reload += change;
			}
		}
		if (!timer || reset) {
			timer = period;
			reset = false;
		} else timer--;
		mute = (reload < 8) || (reload > 0x7FF);
		return reload;
	}
	
};

class Envelope
{
public:
	bool start;
	bool disable;
	bool loop;
	uint16_t divider_count;
	uint16_t volume;
	uint16_t out;
	uint16_t decay_count;

	Envelope() {
		start = disable =  false;
		divider_count = volume = out = decay_count = 0x0000;
	}
	~Envelope();
	void tick() {
		if (!start) {
			if (!divider_count) {
				divider_count = volume;
				if (decay_count == 0) {
					if (loop)
						decay_count = 15;
				} else decay_count--;
			} else divider_count--;
		} else {
			start = false;
			decay_count = 15;
			divider_count = volume;
		}
		if (disable) out = volume;
		else out = decay_count;
	}
	
};

class Sequencer
{
private:
	Envelope * envelope;
	Sweeper * sweeper;
	LinearCounter * linear_counter;
	uint32_t sequence;
	uint16_t counter;
	uint8_t dwn_counter;
	long timer;
	uint16_t reload;
	uint16_t out;
	float duty;
	bool enable;
	bool halt;
	bool channel;
public:
	Sequencer(bool channel) {
		sequence = timer = counter = dwn_counter = reload = halt = out = duty = enable = 0;
		this->channel = channel;
		envelope = new Envelope();
		sweeper = new Sweeper();
		linear_counter = new LinearCounter();
	}
	~Sequencer();
	uint8_t getOut() {return out;}
	uint16_t getRel() {return reload;}
	float getDut() {return duty;}
	bool getEn() {return enable;}
	bool getHlt() {return halt;}
	uint16_t getCnt() {return counter;}
	int getDct() {return dwn_counter;}
	uint16_t getEnv() {return envelope->out;}
	bool getMut() {return sweeper->mute;}
	uint16_t getLiCnt() {return this->linear_counter->counter;}
	double getFrq() {return 1789773.0 / (16.0 * (double)(reload + 1));}
	double getTriFrq() {return getFrq()/2.0;}
	double getAmp() {return (double)(envelope->out-1.0)/16.0;}
	void setOut(Sint16 out) {this->out = out;}
	void setSeq(uint32_t sequence) {this->sequence = sequence;}
	void setDut(float duty) {this->duty = duty;}
	void setHlt(bool halt) {this->halt = halt;}
	void setDct(uint8_t dwn_counter) {this->dwn_counter = dwn_counter;}
	void setVlm(uint16_t volume) {this->envelope->volume = volume;}
	void setDis(bool disable) {this->envelope->disable = disable;}
	void setStr(bool start) {this->envelope->start = start;}
	void setSpEn(bool enable) {this->sweeper->enable = enable;}
	void setPr(uint8_t period) {this->sweeper->period = period;}
	void setDn(bool down) {this->sweeper->down = down;}
	void setSh(uint8_t shift) {this->sweeper->shift = shift;}
	void setRst(bool reset) {this->sweeper->reset = reset;}
	void setLiRel(uint16_t reset_val) {this->linear_counter->reset_val = reset_val;}
	void setLiCnt(bool control) {this->linear_counter->control = control;}
	void setLiRst(bool reset) {this->linear_counter->reset = reset;}
	void setRelUp(uint8_t val) {reload = ((reload&0xFF00) | val);}
	void setRelDn(uint8_t val) {
		reload = (((uint16_t)(val&0x07)<<8) | (reload&0x00FF));
		counter = reload + 1;
	}
	void setEn(uint8_t val) {
		enable = val;
		if (!val) dwn_counter = 0;
	}
	long getTime() {
		long retval = timer;
		timer += getFrq();
		return retval;
	}
	long getTimeTri() {
		long retval = timer;
		timer += getTriFrq();
		return retval;
	}
	void track() {
		sweeper->track(reload);
	}
	void tick_swp() {
		reload = sweeper->tick(reload, channel);
	}
	void tick_env() {
		envelope->loop = halt;
		envelope->tick();
	}
	void tick_cnt() {
		if (!enable)
			dwn_counter = 0;
		else if (dwn_counter > 0 && !halt)
			dwn_counter--;
	}
	void tick_li_cnt() {
		linear_counter->tick();
	}
	void tick(std::function<void(uint32_t &x)> f) {
		if (enable) {
			counter--;
			if (counter = 0xFFFF) {
				counter = reload + 1;
				f(sequence);
				out = sequence & 0x00000001;
			}
		}
	}
	
};

class APU
{
private:

	uint8_t length_table[4*8];

	double time_per_sample = 1.0/(double)FREQUENCY;
	double time_per_ppu_cycle = 1.0/5369318.0;
	double elapsed;
	int counter;

	Sint16 audio_buffer[BUFFER_SIZE] = { 0 };
	Sint16 current_sample;
	int buffer_read, buffer_write;
	long v;
	float frq, dty;
	uint32_t current_cycle;

	double pulse1_sample;
	Sequencer * pulse1_seq;
	Sequencer * pulse2_seq;
	Sequencer * triang_seq;

public:
	APU();
	~APU();
	void tick();
	void write(uint16_t addr, uint8_t data);
	uint8_t read(uint16_t addr);
	bool bufferWrite(Sint16 value);
	bool bufferRead(Sint16 * sample);
	void generateSample();
	void getSamples(Sint16 *stream, int length);
	Sint16 remixyeah();
	
};

#endif