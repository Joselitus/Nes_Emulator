#ifndef MAPPER_H
#define MAPPER_H

#include <cstdint>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
//for debuging
#include <iostream>

#define SAVE_FILE_EXTENSION "save"

class Mapper
{
public:
	Mapper();
	virtual uint32_t getAddr(uint16_t addr) = 0;
	virtual uint32_t getAddrCHR(uint16_t addr) = 0;
	virtual void writeM(uint16_t addr, uint8_t data) = 0;
	virtual int changesMirr() = 0;
	virtual bool interrputs() = 0;
	virtual void updateIrq() = 0;
};

class MROM: public Mapper {
private:
	int prg_size;
public:
	MROM(int prg_size);
	~MROM();
	int changesMirr();
	bool interrputs();
	void updateIrq();
	uint32_t getAddr(uint16_t addr);
	uint32_t getAddrCHR(uint16_t addr);
	void writeM(uint16_t addr, uint8_t data);
};

class CMM1: public Mapper {
private:
	std::string filename;
	bool state;
	int prg_size, chr_size, seq_shift;
	uint8_t cntrl, load, chr0, chr1, prg;
	uint8_t memory[8*1024];
public:
	CMM1(int prg_size, int chr_size, std::string filename);
	~CMM1();
	int changesMirr();
	bool interrputs();
	void updateIrq();
	uint32_t getAddr(uint16_t addr);
	uint32_t getAddrCHR(uint16_t addr);
	void writeM(uint16_t addr, uint8_t data);
	void loadSave();
	void save();
};

class UxROM: public Mapper {
private:
	uint8_t bank_selected;
	int prg_size;
public:
	UxROM(int prg_size);
	~UxROM();
	int changesMirr();
	bool interrputs();
	void updateIrq();
	uint32_t getAddr(uint16_t addr);
	uint32_t getAddrCHR(uint16_t addr);
	void writeM(uint16_t addr, uint8_t data);
};

class CMM3: public Mapper {
private:
	bool state, mirr, prg_mode, chr_mode, irq_enable, act_int;
	int prg_size, chr_size, irq_count;
	uint8_t rgs_slct, irq_latch;
	uint8_t regs[8];
	uint8_t memory[8*1024];
public:
	CMM3(int prg_size, int chr_size);
	~CMM3();
	int changesMirr();
	bool interrputs();
	void updateIrq();
	uint32_t getAddr(uint16_t addr);
	uint32_t getAddrCHR(uint16_t addr);
	void writeM(uint16_t addr, uint8_t data);
};

class MP184: public Mapper {
private:
	uint8_t bank_selected;
	int prg_size;
public:
	MP184(int prg_size);
	~MP184();
	int changesMirr();
	bool interrputs();
	void updateIrq();
	uint32_t getAddr(uint16_t addr);
	uint32_t getAddrCHR(uint16_t addr);
	void writeM(uint16_t addr, uint8_t data);
};

#endif