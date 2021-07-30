#include "Mapper.h"

Mapper::Mapper() {

}
// Mapper 0 ---------------------------------------------
MROM::MROM(int prg_size) {
	this->prg_size = prg_size;
}

int MROM::changesMirr() {
	return -1;
}

bool MROM::interrputs() {
	return false;
}

void MROM::updateIrq() {

}

uint32_t MROM::getAddr(uint16_t addr) {
	if (addr > 0x7FFF) {
		return addr%(prg_size > 16*1024 ? 0x8000 : 0x4000);
	}
	
	else return addr;
}

uint32_t MROM::getAddrCHR(uint16_t addr) {
	return addr;
}

void MROM::writeM(uint16_t addr, uint8_t data) {

}

// Mapper 1 -----------------------------------------
CMM1::CMM1(int prg_size, int chr_size, std::string filename) {
	this->filename = filename;
	this->state = false;
	this->prg_size = prg_size;
	this->chr_size = chr_size;
	this->seq_shift = 0;
	this->cntrl = 0x1C;
	load = chr0 = chr1 = prg = 0x00;
	loadSave();
}
int CMM1::changesMirr() {
	//std::cout << "esto es lo que hay en ctrl" << (uint16_t)cntrl << std::endl;
	if (cntrl&0x02) // horizontal / vertical
		return (unsigned char)(~((cntrl&0x01)|0xFE));
	else 			// lower / upper bank
		return (unsigned char)(cntrl&0x01)+2;
}

bool CMM1::interrputs() {
	return false;
}

void CMM1::updateIrq() {

}

uint32_t CMM1::getAddr(uint16_t addr) {
	if (addr >= 0x6000 && addr < 0x8000) {
		if (state) {
			state = false;
			return memory[addr&0x1FFF];
		}
		else {
			state = true;
			return 0xFFFFFFFF;
		}
	}
	if (((cntrl&0x0F)>>2) < 2) // 32K mode
		return (uint32_t)(addr%0x8000) + ((prg&0x0E)>>1)*32*1024;

	else if (((cntrl&0x0F)>>2) == 2) { // 16K modes
		if (addr < 0xC000)
			return (uint32_t)(addr%(16*1024));
		if (addr >= 0xC000)
			return (uint32_t)(addr%(16*1024)) + (prg&0x0F)*16*1024;
	}

	else {
		if (addr < 0xC000)
			return (uint32_t)(addr%(16*1024)) + (prg&0x0F)*16*1024;
		if (addr >= 0xC000)
			return (uint32_t)(addr%(16*1024)) + prg_size - 16*1024;
	}
	return addr;
}

uint32_t CMM1::getAddrCHR(uint16_t addr) {
	if (!chr_size) return addr;
	if (cntrl&0x10) {
		if (addr < 0x1000)
			return (uint32_t)(addr%(1024*4)) + chr0*1024*4;
		else if (addr < 0x2000)
			return (uint32_t)(addr%(1024*4)) + chr1*1024*4;
	}
	else if (addr < 0x2000) {
		return (uint32_t)(addr%(1024*8)) + ((chr0&0xFE))*1024*8;
	}
	return addr;
}

void CMM1::writeM(uint16_t addr, uint8_t data) {
	if (addr >= 0x6000 && addr < 0x8000) {
		memory[addr&0x1FFF] = data;
	}
	else if (addr >= 0x8000) {
		if (data&0x80)  {
			seq_shift = 0;
			load = 0x00;
			cntrl |= 0x0C;
		}
		else {
			load |= ((data&0x01)<<seq_shift);
			if (++seq_shift == 5) {
				seq_shift = 0;
				switch ((addr>>13)&0x03) {
					case 0:
						cntrl = load;
					break;
					case 1:
						chr0 = load;
					break;
					case 2:
						chr1 = load;
					break;
					case 3:
						prg = load;
					break;
				}
				load = 0x00;
			}
		}
	}
}

void CMM1::save() {
	std::string savename = filename + "." + SAVE_FILE_EXTENSION;
	int file;
	if ((file = open(savename.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		return;
	write(file, memory, 8*1024);
}

void CMM1::loadSave() {
	std::string savename = filename + "." + SAVE_FILE_EXTENSION;
	int file;
	if ((file = open(savename.c_str(), O_RDONLY)) < 0)
		return;
	read(file, memory, 8*1024);
}

// Mapper 2 ------------------------------------------
UxROM::UxROM(int prg_size) {
	this->prg_size = prg_size;
	this->bank_selected = 0;
}

int UxROM::changesMirr() {
	return -1;
}

bool UxROM::interrputs() {
	return false;
}

void UxROM::updateIrq() {

}

uint32_t UxROM::getAddr(uint16_t addr) {
	if (addr >= 0xC000) {
		return (uint32_t)(addr%(16*1024)) + (prg_size-16*1024);
	}
	
	else 
		return (uint32_t)(addr%(16*1024)) + (bank_selected*16*1024);
}

uint32_t UxROM::getAddrCHR(uint16_t addr) {
	return addr;
}

void UxROM::writeM(uint16_t addr, uint8_t data) {
	if (addr >= 0x8000)
		this->bank_selected = data&0x0F;
}

// Mapper 3 -------------------------------------------------------
CMM3::CMM3(int prg_size, int chr_size) {
	this->prg_size = prg_size;
	state = chr_mode = prg_mode = irq_enable = act_int = false;
	mirr = true;
	irq_count = 0;
	rgs_slct = irq_latch = 0x00;
	for (int i = 0; i < 8; i++)
		regs[i] = 0;
	regs[7] = 1;
}
int CMM3::changesMirr() {
	if (mirr) return 0;
	else return 1;
}

bool CMM3::interrputs() {
	bool retval = act_int;
	act_int = false;
	return retval;
}

void CMM3::updateIrq() {
	if (!irq_count)
		irq_count = irq_latch;
	else
		irq_count--;
	if ((!irq_count) && irq_enable)
		act_int = true;
}

uint32_t CMM3::getAddr(uint16_t addr) {
	if (addr >= 0x6000 && addr < 0x8000) {
		if (state) {
			state = false;
			return memory[addr&0x1FFF];
		}
		else {
			state = true;
			return 0xFFFFFFFF;
		}
	}
	if (addr >= 0x8000 && addr < 0xA000) {
		if (prg_mode)
			return (uint32_t)(addr & 0x1FFF) + prg_size - 2*1024*8;
		else
			return (uint32_t)(addr & 0x1FFF) + (regs[6]&0x3f)*1024*8;
	}
	if (addr >= 0xA000 && addr < 0xC000)
		return (uint32_t)(addr & 0x1FFF) + (regs[7]&0x3f)*1024*8;
	if (addr >= 0xC000 && addr < 0xE000) {
		if (prg_mode)
			return (uint32_t)(addr & 0x1FFF) + (regs[6]&0x3f)*1024*8;
		else
			return (uint32_t)(addr & 0x1FFF) + prg_size - 2*1024*8;
	}
	if (addr >= 0xE000)
		return (uint32_t)(addr & 0x1FFF) + prg_size - 1024*8;
	return addr;
}

uint32_t CMM3::getAddrCHR(uint16_t addr) {
	if (addr < 0x0400) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + regs[2]*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[0]&0xFE)*1024;
	}
	if (addr >= 0x0400 && addr < 0x0800) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + regs[3]*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[0]+1)*1024;
	}
	if (addr >= 0x0800 && addr < 0x0C00) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + regs[4]*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[1]&0xFE)*1024;
	}
	if (addr >= 0x0C00 && addr < 0x1000) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + regs[5]*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[1]+1)*1024;
	}
	if (addr >= 0x1000 && addr < 0x1400) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + (regs[0]&0xFE)*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[2])*1024;
	}
	if (addr >= 0x1400 && addr < 0x1800) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + (regs[0]+1)*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[3])*1024;
	}
	if (addr >= 0x1800 && addr < 0x1C00) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + (regs[1]&0xFE)*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[4])*1024;
	}
	if (addr >= 0x1C00) {
		if (chr_mode)
			return (uint32_t)(addr%(1024)) + (regs[1]+1)*1024;
		else
			return (uint32_t)(addr%(1024)) + (regs[5])*1024;
	}
	return addr;
}

void CMM3::writeM(uint16_t addr, uint8_t data) {
	if (addr >= 0x6000 && addr < 0x8000) {
		memory[addr&0x1FFF] = data;
	}
	// std::cout << "escribe en" << std::endl;
	// std::cout << std::hex << addr << std::endl;
	// std::cout << std::hex << (uint16_t)data << std::endl;
	// std::cin.get();
	if (addr >= 0x8000 && addr < 0xA000) {

		if (addr&0x0001) 
			regs[rgs_slct] = data;
		else {
			rgs_slct = data&0x07;
			prg_mode = data&0x40;
			chr_mode = data&0x80;

		}
	}
	if (addr >= 0xA000 && addr < 0xC000) {
		if (addr&0x0001); // Kindly ignore
		else
			mirr = data&0x01;
	}
	if (addr >= 0xC000 && addr < 0xE000) {
		if (addr&0x0001)
			irq_count = 0;
		else
			irq_latch = data;
	}
	if (addr >= 0xE000) {
		if (addr&0x0001) {
			irq_enable = true;
		}
		else {
			irq_enable = false;
			act_int = false;
		}
	}
}

// Mapper 184 ------------------------------------

MP184::MP184(int prg_size) {
	this->prg_size = prg_size;
	this->bank_selected = 0x40;
}

int MP184::changesMirr() {
	return -1;
}

bool MP184::interrputs() {
	return false;
}

void MP184::updateIrq() {

}

uint32_t MP184::getAddr(uint16_t addr) {
	if (addr > 0x7FFF) {
		return addr%(prg_size > 16*1024 ? 0x8000 : 0x4000);
	}
	return addr;
}
uint32_t MP184::getAddrCHR(uint16_t addr) {
	//return addr;
	if (addr < 0x1000)
		return (uint32_t)(addr%(4*1024)) + (((bank_selected&0x0F)%4)*4*1024);
	if (addr >= 0x1000 && addr < 0x2000) {
		return (uint32_t)(addr%(4*1024)) + ((((bank_selected&0xF0)>>4)%4)*4*1024);
	}
	return addr;
}

void MP184::writeM(uint16_t addr, uint8_t data) {
	if (addr >= 0x6000 && addr <= 0x7FFF)
		this->bank_selected = ((data & 0x77)|0x40);
}