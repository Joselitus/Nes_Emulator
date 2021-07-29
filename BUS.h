#ifndef BUS_H
#define BUS_H

#include <iostream>
#include <cstdint>
#include <array>

#include "ROM.h"
#include "APU.h"

#define DISPLAY_X 16
#define DISPLAY_Y 16

#define SUSPENSION_TIME 256 //cycles

#define MEM_SIZE 0x0800 //2K
#define PPU_SIZE 0x0008
#define APU_SIZE 0x0018
#define MEM_ADDR_SIZE 0x2000 //8K
#define PPU_ADDR_SIZE 0x2000 //8k
#define APU_ADDR_SIZE 0x0018

class PPU;

class BUS {
private:
	bool nmi;
	int sus_cycle;
	uint8_t trans_page;
	uint8_t mem[MEM_SIZE] = { 0 };
	PPU * ppu;
	ROM * rom;
	APU * apu;

	uint8_t controller;
	uint8_t controller_reg;

public:
	BUS(PPU * ppu, APU * apu, ROM * rom);
	~BUS();
	
	void write(uint16_t addr, uint8_t data);
	uint8_t read(uint16_t addr);
	uint8_t readCHR(uint16_t addr);
	void writeCHR(uint16_t addr, uint8_t data);
	void setNMI();
	bool getNMI();
	void setIRQ();
	bool getIRQ();
	bool getSuspended();
	int getMirroring();

	void resetController();
	void pressUp();
	void pressDn();
	void pressRt();
	void pressLt();
	void pressSl();
	void pressSt();
	void pressA();
	void pressB();
	void releaseUp();
	void releaseDn();
	void releaseRt();
	void releaseLt();
	void releaseSl();
	void releaseSt();
	void releaseA();
	void releaseB();

	void display_section(uint16_t addr, uint16_t hlg = 0);
};

#include "PPU.h"

#endif