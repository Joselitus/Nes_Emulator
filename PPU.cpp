#include "PPU.h"

PPU::PPU(Display * dis) {
	this->dis = dis;
	reset();
	uint16_t clr[] = {
		0x333,0x014,0x006,0x326,0x403,0x503,0x510,0x420,0x320,0x120,0x031,0x040,0x022,0x000,0x000,0x000,
		0x555,0x036,0x027,0x407,0x507,0x704,0x700,0x630,0x430,0x140,0x040,0x053,0x044,0x000,0x000,0x000,
		0x777,0x357,0x447,0x637,0x707,0x737,0x740,0x750,0x660,0x360,0x070,0x276,0x077,0x000,0x000,0x000,
		0x777,0x567,0x657,0x757,0x747,0x755,0x764,0x772,0x773,0x572,0x473,0x276,0x467,0x000,0x000,0x000,
	};
	for (int i = 0; i < 16*4; i++)
		colors[i] = clr[i];
}

void PPU::reset() {
	this->v = 0;
	this->t = 0;
	this->PPUSTATUS = 0x00;
	this->PPUCTRL = 0X00;
	this->PPUMASK = 0x00;
	this->slct_palette = 0;
	this->frame_even = true;
	this->lowbyte = false;
	this->data_buffer = 0;
	this->current_cycle = 0;
	this->current_scanline = -1;
}

void PPU::setBus(BUS * bus) {
	this->bus = bus;
}

uint16_t PPU::getColor(uint8_t npalette, uint8_t pixel) {
	return colors[readFrom(0x3F00 + (npalette << 2) + pixel) & 0x3F];
}

void PPU::displayChar() {
	for (int tileY = 0; tileY < 32; tileY++) {
		for (int tileX = 0; tileX < 16; tileX++) {
			for (int y = 0; y < 8; y++) {
				uint8_t low = readFrom(tileY*16*16 + tileX*16 + y);
				uint8_t high = readFrom(tileY*16*16 + tileX*16 + y + 8);
				for (int x = 0; x < 8; x++) {
					uint8_t indx = ((low&0x80) >> 7)  + ((high&0x80) >> 6);
					//uint16_t color = colors[indx];
					uint16_t color = getColor(slct_palette, indx);
					//std::cout << ((low&0x80) >> 7)  + (high&0x80 >> 6) << std::endl;
					dis->setPixel(tileX*8+x, tileY*8+y, color);
					low <<= 1;
					high <<= 1;
				}
			}
		}
	}
}

void PPU::displayVram() {
	std::cout << "--------------------------------------------------------------------------------------------------" << std::endl;
	for (int i = 0; i < VRAM_SIZE/2; i++) {
		if (!(i%32)) std::cout << std::endl;
		std::cout << std::hex << "0x" << (uint16_t)vram[i] << ", ";
	}
	std::cout << std::endl << "--------------------------------------------------------------------------------------------------" << std::endl;
}

void PPU::displayOAM() { return;
	std::cout << "----------------" << std::endl;
	for (int i = 0; i < NUM_SPRITES_FRAME; i++) {
		for (int j = 0; j < 4; j++)
			std::cout << std::hex << (uint16_t)OAM[i*4+j] << ", ";
		std::cout << std::endl;
	}
	std::cout << "----------------" << std::endl;
}

void PPU::displayPalette(int x, int y, int size, uint8_t plt) {
	for (uint8_t i = 0; i < 4; i++) {
		dis->setRectangle(x+i*size, y, size, size, getColor(plt, i));
	}
}

uint8_t PPU::reverseByte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

void PPU::setAtRegs() { // Check this is too much free style
	int shift = (int)(((((last_v&0x001F) >> 1)&0x0001)) | (((last_v&0x03E0) >> 5)&0x0002));
	last_v = v;
	AT_latch = (AT_latch >> 2*shift)&0x03;
	// if ((v&0x001F)&0x0002) AT_latch >>= 2;
	// if ((v&0x03E0)&0x0004) AT_latch >>= 4;
	at_reg_A = (at_reg_A&0xFF00) | ((AT_latch&0x01)*0xFF);
	at_reg_B = (at_reg_B&0xFF00) | (((AT_latch&0x02)>>1)*0xFF);
	// std::cout << "ciclo: " << std::dec << current_cycle << std::endl;
	// std::cout << "Curse x: " << std::hex << (v&0x001F) << std::endl;
	// std::cout << "la cosa: " << std::hex << (((v&0x001F) >> 1)&0x0001) << std::endl;
}

void PPU::setSprtPixel() {
	uint8_t temp_pxl;
	sprt_zero = false;
	sprt_pixel = 0;
	if (!(PPUMASK&0x10)) return;
	for (int i = curr_sprt_cnt-1; i >= 0; i--) {
		if (!x_sprt[i]) {
			temp_pxl = ((lsb_ptr_sprt[i]&0x80)>>7) | ((msb_ptr_sprt[i]&0x80)>>6);
			if (temp_pxl) {
				if ((i == 0) && sprt_zero_loaded) sprt_zero = true;
				sprt_pixel = temp_pxl;
				sprt_plt = (at_sprt[i]&0x03) | 0x04;
				sprt_prio = !(at_sprt[i]&0x20);
			}
		}
	}
}

void PPU::renderPixel() {
	uint8_t px_plt;
	uint16_t color;
	uint8_t pixel = ((((msb_ptr_reg << fine_x)&0x8000) >> 14) | (((lsb_ptr_reg << fine_x)&0x8000) >> 15));
	setSprtPixel();
	if (sprt_pixel) {
		if (pixel) {
			if (sprt_zero) PPUSTATUS |= 0x40;
			if (sprt_prio) {
				px_plt = sprt_plt;
				pixel = sprt_pixel;
			}
			else
				px_plt = (((at_reg_B << fine_x)&0x8000) >> 14) | (((at_reg_A << fine_x)&0x8000) >> 15);
		}
		else {
			px_plt = sprt_plt;
			pixel = sprt_pixel;
		}
	}
	else if (pixel) {
		px_plt = (((at_reg_B << fine_x)&0x8000) >> 14) | (((at_reg_A << fine_x)&0x8000) >> 15);
	}
	else {
		px_plt = 0x00;
	}
	color = getColor(0x08 | px_plt, pixel);
	dis->setPixel(current_cycle-1, current_scanline, color);
	// if (!((current_cycle-1)%8) || !(current_scanline%8))
	// 	dis->setPixel(current_cycle-1, current_scanline, 0x700);
}

void PPU::renderFetch() {
	switch ( current_cycle%8 ) {
		case 1:
			//std::cout << std::hex << (uint16_t)vram[4] << std::endl;
			NT_latch = readFrom(0x2000 | (v & 0x0FFF));
			// if (current_cycle == 1)
			// std::cout << std::hex << (uint16_t)v << std::endl; 0000 0000 0001 0000
		break;
		case 3:
			AT_latch = readFrom(0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
		break;
		case 5:
			lowBG_latch = readFrom((((uint16_t)(PPUCTRL&0x10) << 8) + ((uint16_t)NT_latch << 4) + ((v&0x7000)>> 12))); 
			if (current_cycle == 13)
		break;
		case 7:
			highBG_latch = readFrom((((uint16_t)(PPUCTRL&0x10) << 8) + ((uint16_t)NT_latch << 4) + ((v&0x7000)>> 12) + 8));
		break;
	}
}

void PPU::spritesFetch() {
	int relpos;
	int sprt;
	int j = 0;
	sprt_zero_loaded = false;
	//#pragma omp parallel for shared(x_sprt, at_sprt, lsb_ptr_sprt, msb_ptr_sprt, curr_sprt_cnt, j) reduction(+j, curr_sprt_cnt)
	for (int i = 0; (i < NUM_SPRITES_FRAME) && (j <= NUM_SPRITES); i++) {
		sprt = i*4;
		relpos = current_scanline - OAM[sprt];
		if ((relpos >= 0) && (relpos <= ( 7 + ((PPUCTRL&0x20)>>5)*7 ))) {
			if (i == 0) sprt_zero_loaded = true;
			if (j == NUM_SPRITES) {
				PPUSTATUS |= 0x20;
				return;
			}
			x_sprt[j] = OAM[sprt+3];
			at_sprt[j] = OAM[sprt+2];
			if ((at_sprt[j]&0x80)) {//vertical flip
				relpos = ( 7 + ((PPUCTRL&0x20)>>5)*7 ) - relpos;
			}
			if ((PPUCTRL&0x20)) { // 8x16 sprite mode
				if (relpos < 8) {
					lsb_ptr_sprt[j] = readFrom(0x1000*(OAM[sprt+1]&0x01) + (OAM[sprt+1]&0xFE)*16 + relpos);
					msb_ptr_sprt[j] = readFrom(0x1000*(OAM[sprt+1]&0x01) + (OAM[sprt+1]&0xFE)*16 + relpos + 8);
				}
				else {
					lsb_ptr_sprt[j] = readFrom(0x1000*(OAM[sprt+1]&0x01) + (OAM[sprt+1]&0xFE)*16 + 16 + (relpos-7));
					msb_ptr_sprt[j] = readFrom(0x1000*(OAM[sprt+1]&0x01) + (OAM[sprt+1]&0xFE)*16 + 16 + (relpos-7) + 8);
				}
			}
			else { // 8x8 sprite mode
				lsb_ptr_sprt[j] = readFrom(0x1000*((PPUCTRL&0x08)>>3) + (OAM[sprt+1])*16 + relpos);
				msb_ptr_sprt[j] = readFrom(0x1000*((PPUCTRL&0x08)>>3) + (OAM[sprt+1])*16 + relpos + 8);
			}
			if (at_sprt[j]&0x40) { // horizontal flip
				lsb_ptr_sprt[j] = reverseByte(lsb_ptr_sprt[j]);
				msb_ptr_sprt[j] = reverseByte(msb_ptr_sprt[j]);
			}

			curr_sprt_cnt = ++j;
		}
	}
}

void PPU::incY() {
	if ((v & 0x7000) != 0x7000)        // if fine Y < 7
	  v += 0x1000;                     // increment fine Y
	else {
	  v &= ~0x7000;                     // fine Y = 0
	  int y = (v & 0x03E0) >> 5;       // let y = coarse Y
	  if (y == 29) {
	    y = 0;                         // coarse Y = 0
	    v ^= 0x0800;                    // switch vertical nametable
	  }
	  else if (y == 31)
	    y = 0;                          // coarse Y = 0, nametable not switched
	  else
	    y += 1;                         // increment coarse Y
	  v = (v & ~0x03E0) | (y << 5);     // put coarse Y back into v
	}
}

bool PPU::tick() {
	if (current_scanline == -1) { // Dummy scanline
		if (current_cycle == 1) {// End of vblank
			PPUSTATUS &= 0x3f;
		}
	}
	if (current_scanline == 241 && current_cycle == 1) { // Begining of vblank
		PPUSTATUS |= 0x80;
		if (PPUCTRL&0x80) {
			bus->setNMI();
		}
	}
	if ((current_scanline >= -1) && (current_scanline <= 239)) {
		if ((current_cycle >= 321) || (current_cycle <= 256))
			renderFetch();

		if ((current_cycle == 256) && (PPUMASK&0x08)) { //Wapping around
			incY();
		} 

		if ((PPUMASK&0x08) && (current_cycle) && ((current_cycle >= 328) || (current_cycle <= 256)) && !(current_cycle%8)) {
			if ((v & 0x001F) == 31) { // if coarse X == 31
			  v &= ~0x001F;           // coarse X = 0
			  v ^= 0x0400;          // switch horizontal nametable
			}
			else
			  v += 1;                // increment coarse X
		}

		if ((current_cycle >= 2 && current_cycle <= 257) || (current_cycle >= 322 && current_cycle <= 337)) {
			msb_ptr_reg <<= 1;
			lsb_ptr_reg <<= 1;
			at_reg_A <<= 1;
			at_reg_B <<= 1;
			if (current_cycle%8 == 1) {
				msb_ptr_reg = (msb_ptr_reg&0xFF00) | highBG_latch;
				lsb_ptr_reg = (lsb_ptr_reg&0xFF00) | lowBG_latch;
				setAtRegs();
			}
		}

		if ((current_cycle == 257) && (PPUMASK&0x08)) 
			v = ((v&0x7BE0) | (t&(~0x7BE0)));

		if (current_cycle == 258) {
			spritesFetch();
		}

		if ((current_cycle == 260) && (PPUMASK&0x18))
			bus->setIRQ();

		if ((current_scanline == -1) && (current_cycle >= 280 ) && (current_cycle <= 304) && (PPUMASK&0x08))
			v = ((v&0x041F) | (t&(~0x041F)));

		if (current_cycle >= 1 && current_cycle <= 256) {
			renderPixel();
			for (int i = 0; i < curr_sprt_cnt; i++) {
				if (x_sprt[i]) {
					x_sprt[i]--;
				}
				else {
					lsb_ptr_sprt[i] <<= 1;
					msb_ptr_sprt[i] <<= 1;
				}
			}
		}

	}

	// if (current_scanline == -1 && current_cycle == 1) {
	// 	displayChar();
	// 	for (uint8_t i = 0; i < 8; i++) {
	// 		if (i == slct_palette)
	// 			dis->setRectangle(265, 20+20*i, 10, 10, 0xFFF);
	// 		else
	// 			dis->setRectangle(265, 20+20*i, 10, 10, 0x000);
	// 		displayPalette(200, 20 + 20*i, 10, i);
	// 	}
	// }
	//if (current_cycle < 8 && current_scanline+1 < 8)
		//dis->setPixel(current_cycle, current_scanline+1, colors[2+((readFrom(current_scanline+1) << current_cycle)&0x80) | ((((readFrom(current_scanline+9) << current_cycle)&0x80)) << 1)]);
		//dis->setPixel(current_cycle, current_scanline+1, (rand()%2 ? 0xFFF : 0));
	current_cycle++;
	if ( current_scanline == -1 ) { // Skip last cycle of scanline -1 if frame is odd
		if (current_cycle == 339 && !frame_even) {
			current_scanline = 0;
			current_cycle = 0;
		}
	}
	if ( current_cycle > 340 ) {
		current_cycle = 0;
		current_scanline++;
		if ( current_scanline > 260 ) {
			current_scanline = -1;
			frame_even = !frame_even;
			dis->refersh();
			return true;
		}
	}
	return false;
}

void PPU::writeTo(uint16_t addr, uint8_t data) {
	if (addr < 0x2000) {
		// std::cout << std::dec << "scanline: " << current_scanline << std::endl;
		// std::cout << std::dec << "cycle: " << (int)current_cycle << std::endl;
		// std::cout << std::hex << addr << std::endl;
		// std::cout << "Trying to write to ROM" << std::endl;
		bus->writeCHR(addr, data);
		return;
	}
	else if (addr < 0x3F00) {
		switch (bus->getMirroring()) {
			case 0: // horizontal mirroring
				addr &= ~0x0400;
				if (addr&0x0800)
					addr -= 0x0400;
			break;
			case 1: // vertical mirroring
				addr &= ~0x0800;
			break;
			case 2: // one screen, low bank
				addr &= ~0x0C00;
			break;
			case 3: // one screen, upper bank
				addr &= ~0x0800;
				addr |= 0x0400;
			break;
		}
		vram[addr%VRAM_SIZE] = data;
	}
	else if (addr < 0x4000) {
		addr %= PALETE_SIZE;
		if (!(addr%4)) addr &= 0x000f;
		palette[addr] = data;
	}
}

uint8_t PPU::readFrom(uint16_t addr) {
	if (addr < 0x2000)
		return bus->readCHR(addr);
	if (addr < 0x3F00) {
		switch (bus->getMirroring()) {
			case 0: // horizontal mirroring
				addr &= ~0x0400;
				if (addr&0x0800)
					addr -= 0x0400;
			break;
			case 1: // vertical mirroring
				addr &= ~0x0800;
			break;
			case 2: // one screen, low bank
				addr &= ~0x0C00;
			break;
			case 3: // one screen, upper bank
				addr &= ~0x0800;
				addr |= 0x0400;
			break;
		}
		return vram[addr%VRAM_SIZE];
	}
	if (addr < 0x4000) {
		addr %= PALETE_SIZE;
		if (!(addr%4)) addr &= 0x000f;
		return palette[addr];
	}
	return 0;
}

void PPU::write(uint16_t addr, uint8_t data) {
	switch (addr) {
		case 0x0:
			PPUCTRL = data;
			t = (t&0x73FF) | ((uint16_t)(data&0x03) << 10);
		break;
		case 0x1:
			PPUMASK = data;
		break;
		case 0x2:
		break;
		case 0x3:
			OAMADDR = data;
		break;
		case 0x4:
			OAM[OAMADDR] = data;
		break;
		case 0x5: //Scroll
			if (lowbyte) // t = 0FGH..AB CDE..... <- data: ABCDEFGH
				t = (t&0x0C1F) | 
					((uint16_t)data << 12) | 
					((uint16_t)(data&0xC0) << 2) | 
					(uint16_t((data<<2)&0xE0));
			else {
				t = (t&0x7FE0) | (uint16_t)(data >> 3);
				fine_x = data&0x07;
			}
			lowbyte = !lowbyte;
		break;
		case 0x6: //Address
			// std::cout << "changing the address on scanline: " << std::endl;
			// std::cout << std::dec << current_scanline << std::endl;
			// std::cout << "mask is: " << std::hex << (PPUMASK&0x08) << std::endl;
			if (lowbyte) {
				t = (t&0xFF00) | (uint16_t)data;
				v = t;
			}
			else
				t = (t&0x00FF) | (uint16_t)data << 8;
			lowbyte = !lowbyte;
			addr %= 0x3FFF;
		break;
		case 0x7: //Data
			// std::cout << "si, era yo todo este tiempo" << std::endl;
			// std::cout << "estoy vblanqueando? " << current_scanline << std::endl;
			writeTo(v, data);
			//std::cout << std::hex << t << std::endl;
			v += 31*((PPUCTRL & 0X04)>>2) + 1;
		break;
	}
}

uint8_t PPU::read(uint16_t addr) {
	uint8_t retval;
	switch (addr) {
		case 0x0:
			return PPUCTRL;
		break;
		case 0x1:
			return PPUMASK;
		break;
		case 0x2: //Status
			// std::cout << "he leido de status: " << std::hex << (uint16_t)PPUSTATUS << std::endl;
			// std::cout << "en la linea: " << std::dec << current_scanline << std::endl;
			retval = PPUSTATUS;
			PPUSTATUS &= 0x7f;
			lowbyte = false;
			return retval;
		break;
		case 0x3:
		break;
		case 0x4:
			return OAM[OAMADDR];
		break;
		case 0x5:
		break;
		case 0x6: //Address
		break;
		case 0x7: //Data
			retval = data_buffer;
			data_buffer = readFrom(v);
			v += 31*(PPUCTRL & 0X04) + 1;
			if (v >= 0x3F00) return data_buffer;
			return retval;
		break;
	}
	return 0;
}

void PPU::writeOAM(uint8_t addr, uint8_t data) {
	OAM[addr] = data;
}

void PPU::changePalette(uint8_t plt) {
	if (plt > 7) {
		this->slct_palette = (slct_palette+1)%8;
		return;
	}
	this->slct_palette = plt;
}