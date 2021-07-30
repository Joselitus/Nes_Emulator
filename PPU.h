#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <signal.h>
#include <cstddef>

#include "Display.h"
#include "BUS.h"
#include "ROM.h"

#define NUM_COLORS 64
#define NUM_SPRITES 8
#define NUM_SPRITES_FRAME 64
#define VRAM_SIZE 0x0800
#define	PALETE_SIZE 0x0020
#define OAM_SIZE 0x100

class PPU
{
private:

	// Sprites registers
	uint8_t x_sprt[NUM_SPRITES];
	uint8_t at_sprt[NUM_SPRITES];
	uint8_t msb_ptr_sprt[NUM_SPRITES];
	uint8_t lsb_ptr_sprt[NUM_SPRITES];
	uint8_t curr_sprt_cnt;
	uint8_t sprt_pixel;
	uint8_t sprt_plt;
	bool sprt_prio;
	bool sprt_zero;
	bool sprt_zero_loaded;

	// Background registers
	bool lowbyte;
	uint8_t data_buffer;
	uint8_t slct_palette;
	uint8_t NT_latch;
	uint8_t AT_latch;
	uint8_t lowBG_latch;
	uint8_t highBG_latch;
	uint16_t msb_ptr_reg;
	uint16_t lsb_ptr_reg;
	uint16_t at_reg_A;
	uint16_t at_reg_B;

	// Loopy registers
	uint16_t v;
	uint16_t t;
	uint8_t fine_x;

	// PPU registers
	uint8_t PPUCTRL;
	uint8_t PPUMASK;
	uint8_t PPUSTATUS;
	uint8_t OAMADDR;

	uint16_t colors[NUM_COLORS];
	uint8_t vram[VRAM_SIZE] = { 0 };
	uint8_t palette[PALETE_SIZE] = { 0 };
	uint8_t OAM[OAM_SIZE] = { 0 };
	int current_cycle;
	int current_scanline;
	bool frame_even;

	BUS * bus;
	Display * dis;
	void writeTo(uint16_t addr, uint8_t data);
	uint8_t readFrom(uint16_t addr);
	void renderFetch();
	void spritesFetch();
	void setAtRegs();
	void setSprtPixel();
	void renderPixel();
	void incY();
	uint8_t reverseByte(uint8_t b);
public:
	PPU(Display * dis);
	~PPU();
	void setBus(BUS * bus);
	void reset();
	uint16_t getColor(uint8_t npalette, uint8_t pixel);
	bool tick();
	void wait(int cicles);
	void write(uint16_t addr, uint8_t data);
	uint8_t read(uint16_t addr);
	void writeOAM(uint8_t addr, uint8_t data);

	// Debug
	void displayOAM();
	void displayChar();
	void displayVram();
	void displayPalette(int x, int y, int size, uint8_t plt);
	void changePalette(uint8_t plt);
};

#endif