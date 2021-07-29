#ifndef ROM_H
#define ROM_H

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

#include "Mapper.h"

#define HEADER_SIZE 16
#define TRAINER_SIZE 512

#define F0 0x01
#define F1 0x02
#define F2 0x04
#define F3 0x08

class ROM
{
private:

	std::string filename;

	uint8_t mapperID;
	Mapper * mapper;

	int prg_size;
	int chr_size;
	uint8_t * prg;
	uint8_t * chr;

	bool mirroring_arrangement; // 0 horizontal 1 vertical
	bool battery_backed;
	bool contains_trainer;
	bool ignore_mirroring;
	bool uv_unisys;
	bool has_chr;

	bool parseHeader(uint8_t * buff);
	void asignMapper();
public:
	ROM();
	~ROM();

	bool load(std::string filename);
	void reset();
	uint8_t readCHR(uint16_t addr);
	uint8_t readPRG(uint16_t addr);
	void writeCHR(uint16_t addr, uint8_t data);
	void writePRG(uint16_t addr, uint8_t data);
	int getMirroring();
	void setIRQ();
	bool getIRQ();
	
};

#endif