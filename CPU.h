#ifndef CPU_H
#define CPU_H

#include <cstdint>

#include <iostream>
using namespace std;

#include "BUS.h"

// flag position
#define C 0x01
#define Z 0x02
#define I 0x04
#define D 0x08
#define B 0x10
#define U 0x20
#define V 0x40
#define N 0x80

// Highest bit in an 8 bit register
#define HGB 0x80
// Page reserved for stack
#define STACK_PAGE 0x0100
// Pointer to the interrupt address
#define INTERRUPT_ADDR 0xFFFE
// Pointer to the interrupt address (when non maskable)
#define INTERRUPT_ADDR_NM 0xFFFA
// Maximum numeric value of unsigned 8 bit register
#define MAX_BYTE 255

class CPU {

private:

	sigset_t set;
	int sig;
	bool stop;

	int extra_cycles;

	uint8_t A;
	uint8_t X, Y;
	uint16_t PC;
	uint8_t S;
	uint8_t P;

	uint16_t operand_addr;
	bool operand_is_a;
	uint8_t operand;


	BUS * bus;

	void setF(uint8_t flag, int state);
	bool getF(uint8_t flag);

	int branchRel();

	void push(uint8_t byte);
	uint8_t pop();

	void pushWord(uint16_t word);
	uint16_t popWord();

	void readOperand();
	void writeBack();

	void execute(uint8_t opcode);

public: 
	bool dothething;
	int cycles;
	CPU(BUS * bus);
	~CPU();

	// Addressing modes
	void ACC(); void IMM(); void REL(); void ABS(); 
	void ZDP(); void IND(); void ABX(); void ABY(); 
	void ZPX(); void ZPY(); void IIX(); void IIY();


	// Instructions
	void ADC(); void AND(); void ASL(); void BCC(); 
	void BCS(); void BEQ(); void BIT(); void BMI(); 
	void BNE(); void BPL(); void BRK(); void BVC();
	void BVS(); void CLC(); void CLD(); void CLI(); 
	void CLV(); void CMP(); void CPX(); void CPY(); 
	void DEC(); void DEX(); void DEY(); void EOR();
	void INC(); void INX(); void INY(); void JMP(); 
	void JSR(); void LDA(); void LDX(); void LDY(); 
	void LSR(); void NOP(); void ORA(); void PHA();
	void PHP(); void PLA(); void PLP(); void ROL(); 
	void ROR(); void RTI(); void RTS(); void SBC();
	void SEC(); void SED(); void SEI(); void STA(); 
	void STX(); void STY(); void TAX(); void TAY(); 
	void TSX(); void TXA(); void TXS(); void TYA();

	void NMI();
	void IRQ();
	void reset();
	void tick();
	
};

#endif