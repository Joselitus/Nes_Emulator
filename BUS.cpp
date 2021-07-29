#include "BUS.h"

BUS::BUS(PPU * ppu, APU * apu, ROM * rom) {
	this->controller = 0x00;
	this->nmi = false;
	this->sus_cycle = SUSPENSION_TIME;
	this->rom = rom;
	this->ppu = ppu;
	this->apu = apu;
	ppu->setBus(this);
}

void BUS::write(uint16_t addr, uint8_t data) {
	//std::cout << std::hex << (uint16_t)controller << std::endl;
	if (addr < MEM_ADDR_SIZE)
		this->mem[addr%MEM_SIZE] = data;
	else if (addr < MEM_ADDR_SIZE + PPU_ADDR_SIZE) {
		this->ppu->write(addr%PPU_SIZE, data);
	}
	else if (addr < MEM_ADDR_SIZE + PPU_ADDR_SIZE + APU_ADDR_SIZE) {
		if (addr == 0x4014) {
			sus_cycle = 0;
			trans_page = data;
		}
		else if (addr == 0x4016) {
			controller_reg = controller;
		}
		else
			this->apu->write(addr, data);
	}
	else
		rom->writePRG(addr, data);
}

uint8_t BUS::read(uint16_t addr) {
	if (addr < MEM_ADDR_SIZE)
		return this->mem[addr%MEM_SIZE];
	else if (addr < MEM_ADDR_SIZE + PPU_ADDR_SIZE) {
		return this->ppu->read(addr%PPU_SIZE);
	}
	else if (addr < MEM_ADDR_SIZE + PPU_ADDR_SIZE + APU_ADDR_SIZE) {
		if (addr == 0x4016) {
			uint8_t retval = (controller_reg&0x80) >> 7;
			controller_reg <<= 1;
			//std::cout << "al menos lo intenta" << std::endl;
			return retval;
		}
		else
			return this->apu->read(addr);
	}
	else {
		return rom->readPRG(addr);
	}
}

uint8_t BUS::readCHR(uint16_t addr) {
	return rom->readCHR(addr);
}

void BUS::writeCHR(uint16_t addr, uint8_t data) {
	return rom->writeCHR(addr, data);
}

void BUS::setNMI() {
	this->nmi = true;
}

bool BUS::getNMI() {
	bool retval = this->nmi;
	this->nmi = false;
	return retval;
}

void BUS::setIRQ() {
	this->rom->setIRQ();
}

bool BUS::getIRQ() {
	return this->rom->getIRQ();
}

bool BUS::getSuspended() {
	if (sus_cycle < SUSPENSION_TIME) {
		uint8_t data = read(((uint16_t)trans_page << 8)+sus_cycle);
		this->ppu->writeOAM(sus_cycle, data);
		sus_cycle++;
		return true;
	}
	else
		return false;
}

void BUS::resetController() {controller = 0x00;}
void BUS::pressA() {controller |= 0x80;}
void BUS::pressB() {controller |= 0x40;}
void BUS::pressSl() {controller |= 0x20;}
void BUS::pressSt() {controller |= 0x10;}
void BUS::pressUp() {controller |= 0x08;}
void BUS::pressDn() {controller |= 0x04;}
void BUS::pressLt() {controller |= 0x02;}
void BUS::pressRt() {controller |= 0x01;}
void BUS::releaseA() {controller &= ~0x80;}
void BUS::releaseB() {controller &= ~0x40;}
void BUS::releaseSl() {controller &= ~0x20;}
void BUS::releaseSt() {controller &= ~0x10;}
void BUS::releaseUp() {controller &= ~0x08;}
void BUS::releaseDn() {controller &= ~0x04;}
void BUS::releaseLt() {controller &= ~0x02;}
void BUS::releaseRt() {controller &= ~0x01;}


void BUS::display_section(uint16_t addr, uint16_t hlg) {
	for (uint16_t i = 0; i < DISPLAY_Y; i++) {
		std::cout << "0x" << std::hex << addr + DISPLAY_Y*i << "	";
		for (uint16_t j = 0; j < DISPLAY_X; j++) {
			if (addr + DISPLAY_Y*i + j == hlg) std::cout << "\033[44m";
			std::cout << ((read(addr + DISPLAY_Y*i + j) < 16) ? '0' : '\0') << std::hex << std::uppercase << unsigned((uint8_t)read(addr + DISPLAY_Y*i + j));
			if (addr + DISPLAY_Y*i + j == hlg) std::cout << "\033[0m";
			std::cout << " ";
		}
		std::cout << std::endl;
	}
}

int BUS::getMirroring() {
	return rom->getMirroring();
}
