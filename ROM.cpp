#include "ROM.h"

ROM::ROM() {

}

ROM::~ROM() {
	if ( prg )
		delete[] prg;
	if ( chr )
		delete[] chr;
}

uint8_t ROM::readCHR(uint16_t addr) {
// 	std::cout << "quiere: " << std::hex << addr << std::endl;
// 	std::cout << "y le dan: " << std::hex << mapper->getAddrCHR(addr) << std::endl;
// 	std::cout << "donde hay: " << std::hex << (uint16_t)chr[mapper->getAddrCHR(addr)] << std::endl;	
	return chr[mapper->getAddrCHR(addr)];
}

void ROM::writeCHR(uint16_t addr, uint8_t data) {
	chr[mapper->getAddrCHR(addr)] = data;
}

uint8_t ROM::readPRG(uint16_t addr) {
	// std::cout << "quiere leer " << std::hex << addr << std::endl; 
	// std::cout << "y lee " << std::hex <<  mapper->getAddr(addr) << std::endl;
	// std::cin.get();
	uint32_t newaddr = mapper->getAddr(addr);
	if (newaddr == 0xFFFFFFFF) // dummy value, means mapper takes controll
		return (uint8_t)mapper->getAddr(addr);
	return prg[newaddr];
}

void ROM::writePRG(uint16_t addr, uint8_t data) {
	mapper->writeM(addr, data);
}

bool ROM::parseHeader(uint8_t * buff) {
	if (! (buff[0] == 'N' && buff[1] == 'E' && buff[2] == 'S' && buff[3] == 0x1A)) {
		std::cout << filename << ": Invalid rom format" << std::endl;
		return false;
	}
	prg_size = (int)buff[4]*16*1024;
	chr_size = (int)buff[5]*8*1024;
	std::cout << chr_size << std::endl;
	std::cout << prg_size << std::endl;
	if (!chr_size) {
		has_chr = false;
		chr_size = 8*1024;
	} else has_chr = true;

	if ( prg )
		delete[] prg;
	if ( chr )
		delete[] chr;
	prg = new uint8_t[prg_size];
	chr = new uint8_t[chr_size];

	uint8_t f6 = buff[6];

	mirroring_arrangement = f6 & F0;
	battery_backed = f6 & F1;
	contains_trainer = f6 & F2;
	ignore_mirroring = f6 & F3;

	uint8_t f7 = buff[7];

	uv_unisys = f7 & F0;

	mapperID = ((f6 & 0xF0) >> 4) | (f7 & 0xF0);

	return true;
}

bool ROM::load(std::string filename) {
	int file, parsed_ok;
	this->filename = filename;
	if ((file = open(filename.c_str(), O_RDONLY)) < 0) {
		std::cout << "Couldn't open the file: " << filename << std::endl;
		return false;
	}
	size_t size = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);
	uint8_t * buff = new uint8_t[size];
	read(file, buff, size);
	if ( (parsed_ok = parseHeader(buff)) ) {
		asignMapper();
		memcpy(prg, buff+HEADER_SIZE+contains_trainer*TRAINER_SIZE, prg_size);
		if (has_chr) {
			memcpy(chr, buff+HEADER_SIZE+contains_trainer*TRAINER_SIZE+prg_size, chr_size);
			//for (int i = 0; i < chr_size; i++)
				//std::cout << std::hex << (uint16_t)chr[i] << std::endl;
		}
	}

	delete[] buff;
	return parsed_ok;
}

void ROM::reset() {
	save();
	load(filename);
}

void ROM::save() {
	CMM1 * cmm1mapper = (CMM1*)mapper;
	cmm1mapper->save();
}

void ROM::asignMapper() {
	switch ( mapperID ) {
		case 0:
			mapper = new MROM(prg_size);
		break;
		case 1:
			mapper = new CMM1(prg_size, has_chr, filename);
		break;
		case 2:
			mapper = new UxROM(prg_size);
		break;
		case 4:
			mapper = new CMM3(prg_size, has_chr);
		break;
		case 184:
			mapper = new MP184(prg_size);
		break;
		default:
			std::cout << "Whoopsy baboopsy parece que no me he molestado en hacer que este juego funcione, difruta del core dumped ;)" << std::endl;
	}
}

int ROM::getMirroring() {
	int mr = this->mapper->changesMirr();
	if ( mr >= 0)
		return mr;
	else 
		return this->mirroring_arrangement;
}

void ROM::setIRQ() {
	this->mapper->updateIrq();
}

bool ROM::getIRQ() {
	return this->mapper->interrputs();
}