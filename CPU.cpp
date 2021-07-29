#include "CPU.h"

CPU::CPU(BUS * bus) {
	this->bus = bus;
	reset();
}

CPU::~CPU() {

}

void CPU::setF(uint8_t flag, int state) {
	if (state) P |= flag;
	else P &= ~flag;
}

bool CPU::getF(uint8_t flag) {
	if (P&flag) return 1;
	return 0;
}

int CPU::branchRel() {
	PC = operand_addr - 1;
	return 1 + (operand_addr & 0xFF00 != PC & 0xFF00);
}

void CPU::push(uint8_t byte) {
	bus->write(STACK_PAGE | (S--), byte);
}

uint8_t CPU::pop() {
	return bus->read(STACK_PAGE | ++S);
}

void CPU::pushWord(uint16_t word) {
	bus->write(STACK_PAGE | (S--), (uint8_t)(word >> 8));
	bus->write(STACK_PAGE | (S--), (uint8_t)(0x00FF & word));
}

uint16_t CPU::popWord() {
	uint16_t val = (bus->read(STACK_PAGE | S+1)) | ((uint16_t)bus->read(STACK_PAGE | S+2) << 8);
	S += 2;
	return val;
}

void CPU::readOperand() {
	if ( operand_is_a )
		operand = A;
	else
		operand = bus->read(operand_addr);
}

void CPU::writeBack() {
	if ( operand_is_a )
		A = operand;
	else
		bus->write(operand_addr, operand);
}

// TODO maybe make operand_is_a false every loop instead of having it
// in each adressing mode?

// Adressing modes
void CPU::ACC() {
	operand_is_a = true;
} 

void CPU::IMM() {
	operand_addr = ++PC;
	operand_is_a = false;
}

void CPU::REL() {
	operand_addr = (int8_t)bus->read(PC+1) + PC + 2;
	PC++;
	operand_is_a = false;
}
	
void CPU::ABS() {
	operand_addr = bus->read(PC+1) | (((uint16_t)bus->read(PC+2)) << 8);
	PC += 2;
	operand_is_a = false;
}

void CPU::ZDP() {
	operand_addr = 0x00FF & bus->read(++PC);
	operand_is_a = false;
}

void CPU::IND() {
	operand_addr = bus->read(PC+1) | (((uint16_t)bus->read(PC+2)) << 8);
	PC += 2;
	if (operand_addr&0x00FF == 0x00FF) // Hardware bug
		operand_addr = bus->read(operand_addr) | (((uint16_t)bus->read(operand_addr&0xFF00)) << 8);
	else 
		operand_addr = bus->read(operand_addr) | (((uint16_t)bus->read(operand_addr+1)) << 8);
	operand_is_a = false;
} 

void CPU::ABX() {
	operand_addr = (bus->read(PC+1) | (((uint16_t)bus->read(PC+2)) << 8)) + X;
	PC += 2;
	operand_is_a = false;
}

void CPU::ABY() {
	operand_addr = (bus->read(PC+1) | (((uint16_t)bus->read(PC+2)) << 8)) + Y;
	PC += 2;
	operand_is_a = false;
} 

void CPU::ZPX() {
	operand_addr = 0x00FF&(bus->read(++PC)+X);
	operand_is_a = false;
} 

void CPU::ZPY() {
	operand_addr = 0x00FF&(bus->read(++PC)+Y);
	operand_is_a = false;
}

void CPU::IIX() { //TODO cuidado con estos
	operand_addr = bus->read(++PC);
	operand_addr = 0x00FF&(operand_addr+X);
	operand_addr = bus->read(operand_addr) | ((uint16_t)bus->read(operand_addr+1) << 8);
	operand_is_a = false;
}

void CPU::IIY() {
	operand_addr = bus->read(++PC);
	operand_addr &= 0x00FF;
	operand_addr = bus->read(operand_addr) | ((uint16_t)bus->read(operand_addr+1) << 8);
	operand_addr += Y;
	operand_is_a = false;
}

// Instructions
//Careful with this one
void CPU::ADC() {
	readOperand();
	uint16_t res = (uint16_t)A + (uint16_t)operand + (uint16_t)getF(C);
	setF(V, (~(A ^ operand) & (A ^ res)) & HGB);
	setF(C, res > MAX_BYTE);
	setF(N, res & HGB);
	setF(Z, !(res&0x00FF));
	A = res&0x00FF;
}

void CPU::AND() {
	readOperand();
	A &= operand;
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::ASL() {
	readOperand();
	setF(C, operand & HGB);
	operand <<= 1;
	setF(N, operand & HGB);
	setF(Z, !operand);
	writeBack();
}

void CPU::BRK() {
	pushWord(PC+2);
	setF(B, 1);
	push(P);
	setF(B, 0);
	setF(I, true);
	PC = (bus->read(INTERRUPT_ADDR) | (uint16_t)(bus->read(INTERRUPT_ADDR+1) << 8)) - 1;
}

void CPU::NMI() {
	pushWord(PC);
	setF(B, false);
	setF(U, true);
	setF(I, true);
	push(P);
	PC = (bus->read(INTERRUPT_ADDR_NM) | (uint16_t)(bus->read(INTERRUPT_ADDR_NM+1) << 8));
}

void CPU::IRQ() {
	if (!getF(I)) {
		pushWord(PC);
		setF(B, false);
		setF(U, true);
		setF(I, true);
		push(P);
		PC = (bus->read(INTERRUPT_ADDR) | (uint16_t)(bus->read(INTERRUPT_ADDR+1) << 8));

	}
}

void CPU::BIT() {
	readOperand();
	setF(Z, !(A & operand));
	setF(N, operand & HGB);
	setF(V, operand & 0x40);

}

// Branch operations:
void CPU::BCC() {
	if ( !getF(C) ) extra_cycles = branchRel();
}

void CPU::BCS() {
	if ( getF(C) ) extra_cycles = branchRel();
}

void CPU::BEQ() {
	if ( getF(Z) ) extra_cycles = branchRel();
}

void CPU::BMI() {
	if ( getF(N) ) extra_cycles = branchRel();
}

void CPU::BNE() {
	if ( !getF(Z) ) extra_cycles = branchRel();
}

void CPU::BPL() {
	if ( !getF(N) ) extra_cycles = branchRel();
}

void CPU::BVC() {
	if ( !getF(V) ) extra_cycles = branchRel();
}

void CPU::BVS() {
	if ( getF(V) ) extra_cycles = branchRel();
}

void CPU::CLC() {
	setF(C, 0);
}

void CPU::CLD() {
	setF(D, 0);
}

void CPU::CLI() {
	setF(I, 0);
}

void CPU::CLV() {
	setF(V, 0);
}

void CPU::CMP() {
	readOperand();
	setF(N, (A - operand) & HGB);
	setF(Z, A == operand);
	setF(C, A >= operand);
}

void CPU::CPX() { 
	readOperand();
	setF(N, (X - operand) & HGB);
	setF(Z, X == operand);
	setF(C, X >= operand);
}

void CPU::CPY() {
	readOperand();
	setF(N, (Y - operand) & HGB);
	setF(Z, Y == operand);
	setF(C, Y >= operand);
}

void CPU::DEC() {
	readOperand();
	operand--;
	setF(N, operand & HGB);
	setF(Z, !operand);
	writeBack();
}

void CPU::DEX() {
	X--;
	setF(N, X & HGB);
	setF(Z, !X);
}

void CPU::DEY() {
	Y--;
	setF(N, Y & HGB);
	setF(Z, !Y);
}

void CPU::EOR() {
	readOperand();
	A ^= operand;
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::INC() {
	readOperand();
	operand++;
	setF(N, operand & HGB);
	setF(Z, !operand);
	writeBack();
}

void CPU::INX() {
	X++;
	setF(N, X & HGB);
	setF(Z, !X);
}

void CPU::INY() {
	Y++;
	setF(N, Y & HGB);
	setF(Z, !Y);
}

void CPU::JMP() {
	PC = operand_addr - 1;
}

void CPU::JSR() {
	pushWord(PC);
	PC = operand_addr - 1;
}

void CPU::LDA() {
	readOperand();
	A = operand;
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::LDX() {
	readOperand();
	X = operand;
	setF(N, X & HGB);
	setF(Z, !X);
}

void CPU::LDY() {
	readOperand();
	Y = operand;
	setF(N, Y & HGB);
	setF(Z, !Y);
}

void CPU::LSR() {
	readOperand();
	setF(C, operand & 0x01);
	operand >>= 1;
	setF(N, 0);
	setF(Z, !operand);
	writeBack();
}

void CPU::NOP() {

}

void CPU::ORA() {
	readOperand();
	A |= operand;
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::PHA() {
	push(A);
}

void CPU::PHP() {
	push(P | 0x10);
}

void CPU::PLA() {
	A = pop();
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::PLP() {
	P = (P & 0x30) | (pop() & 0xCF);
}

void CPU::ROL() {
	readOperand();
	if (getF(C)) {
		setF(C, operand & HGB);
		operand <<= 1;
		operand++;
	} 
	else {
		setF(C, operand & HGB);
		operand <<= 1;
	}
	setF(N, operand & HGB);
	setF(Z, !operand);
	writeBack();
}

void CPU::ROR() {
	readOperand();
	if (getF(C)) {
		setF(C, operand & 0x01);
		operand = (operand >> 1) + HGB;
	}
	else {
		setF(C, operand & 0x01);
		operand >>= 1;
	}
	setF(N, operand & HGB);
	setF(Z, !operand);
	writeBack();
}

void CPU::RTI() {
	P = (P & 0x30) | (pop() & 0xCF);
	PC = popWord() - 1;
}

void CPU::RTS() {
	PC = popWord();
}

void CPU::SBC() {
	readOperand();
	uint16_t value = ((uint16_t)operand) ^ 0x00FF;
	
	uint16_t temp = (uint16_t)A + value + (uint16_t)getF(C);
	setF(C, temp & 0xFF00);
	setF(Z, ((temp & 0x00FF) == 0));
	setF(V, (temp ^ (uint16_t)A) & (temp ^ value) & 0x0080);
	setF(N, temp & 0x0080);
	A = temp & 0x00FF;
}

void CPU::SEC() {
	setF(C, 1);
}

void CPU::SED() {
	setF(D, 1);
}

void CPU::SEI() {
	setF(I, 1);
}

void CPU::STA() {
	bus->write(operand_addr, A);
}

void CPU::STX() {
	bus->write(operand_addr, X);
}

void CPU::STY() {
	bus->write(operand_addr, Y);
}

void CPU::TAX() {
	X = A;
	setF(N, A & HGB);
	setF(Z, !A);
}
void CPU::TAY() {
	Y = A;
	setF(N, A & HGB);
	setF(Z, !A);
}

void CPU::TSX() {
	X = S;
	setF(N, X & HGB);
	setF(Z, !X);
}
void CPU::TXA() {
	A = X;
	setF(N, X & HGB);
	setF(Z, !X);
}

void CPU::TXS() {
	S = X;
}

void CPU::TYA() {
	A = Y;
	setF(N, Y & HGB);
	setF(Z, !Y);
}


void CPU::reset() {
	A = X = Y = 0;
	cycles = 0;
	S = 0xFF;
	P = 0x00 | U;
	PC = bus->read(0xFFFC) | (((uint16_t)bus->read(0xFFFD) << 8));
	extra_cycles = 8;
	dothething = false;
}

void CPU::tick() {
	if (!(bus->getSuspended())) {
		if ( !extra_cycles ) {
			if (bus->getNMI())
				NMI();
			if (bus->getIRQ()) {
				IRQ();
			}
			// cout << "kpder" << endl;
			if (dothething) {
				cout << "Number of instructions: " << cycles << endl;
				cout << "PC: " << hex << PC << " A: " << hex << (uint16_t)A << " X: " << hex << unsigned(X) << " Y: " << hex << unsigned(Y) << endl;
				cout << "N: " << getF(N) << " V: " << getF(V) << " U: " << getF(U) << " B: " << getF(B) << " D: " << getF(D) << " I: " << getF(I) << " Z: " << getF(Z) << " C: " << getF(C) << endl;
				bus->display_section(PC-PC%(DISPLAY_X*DISPLAY_Y), PC);
				bus->display_section(0x8000-0x8000%(DISPLAY_X*DISPLAY_Y), 0x8000);
				bus->display_section(0xA000-0xA000%(DISPLAY_X*DISPLAY_Y), 0xA000);
				bus->display_section(0xC000-0xC000%(DISPLAY_X*DISPLAY_Y), 0xC000);
				bus->display_section(0xE000-0xE000%(DISPLAY_X*DISPLAY_Y), 0xE000);
				cin.get();
			}
			execute(bus->read(PC));
			cycles++;
			PC++;
		}
		else extra_cycles--;
	}
}

void CPU::execute(uint8_t opcode) { // AKA the great abysm


switch (opcode) {


case 0x00:


BRK();
extra_cycles = 6;


break;


case 0x01:


IIX();


ORA();
extra_cycles = 5;


break;


case 0x05:


ZDP();


ORA();
extra_cycles = 2;


break;


case 0x06:


ZDP();


ASL();

extra_cycles = 4;


break;


case 0x08:
PHP();
extra_cycles = 2;

break;


case 0x09:


IMM();


ORA();

extra_cycles = 1;

break;


case 0x0A:


ACC();


ASL();

extra_cycles = 1;

break;


case 0x0D:


ABS();


ORA();

extra_cycles = 3;

break;


case 0x0E:


ABS();


ASL();

extra_cycles = 5;

break;


case 0x10:


REL();
BPL();

extra_cycles = 1;

break;


case 0x11:


IIY();


ORA();

extra_cycles = 4;

break;


case 0x15:


ZPX();


ORA();

extra_cycles = 3;

break;


case 0x16:


ZPX();


ASL();

extra_cycles = 5;


break;


case 0x18:


CLC();

extra_cycles = 1;


break;


case 0x19:


ABY();


ORA();

extra_cycles = 3;

break;


case 0x1D:


ABX();


ORA();

extra_cycles = 3;

break;


case 0x1E:


ABX();


ASL();

extra_cycles = 5;

break;


case 0x20:


ABS();


JSR();

extra_cycles = 5;

break;


case 0x21:


IIX();


AND();

extra_cycles = 5;

break;


case 0x24:


ZDP();


BIT();

extra_cycles = 2;

break;


case 0x25:


ZDP();


AND();

extra_cycles = 2;

break;


case 0x26:


ZDP();


ROL();

extra_cycles = 4;


break;


case 0x28:


PLP();

extra_cycles = 3;

break;


case 0x29:


IMM();


AND();

extra_cycles = 1;

break;


case 0x2A:


ACC();


ROL();

extra_cycles = 1;

break;
case 0x2C:


ABS();


BIT();

extra_cycles = 3;

break;


case 0x2D:


ABS();


AND();

extra_cycles = 3;

break;


case 0x2E:


ABS();


ROL();

extra_cycles = 5;

break;


case 0x30:


REL();


BMI();


break;


case 0x31:


IIY();


AND();


break;
case 0x35:


ZPX();


AND();


break;


case 0x36:


ZPX();


ROL();


break;


case 0x38:


SEC();


break;


case 0x39:


ABY();


AND();


break;


case 0x3D:


ABX();


AND();


break;


case 0x3E:
ABX();


ROL();


break;


case 0x40:


RTI();


break;


case 0x41:


IIX();


EOR();


break;


case 0x45:


ZDP();


EOR();


break;


case 0x46:


ZDP();


LSR();


break;


case 0x48:


PHA();
break;


case 0x49:


IMM();


EOR();


break;


case 0x4A:


ACC();


LSR();


break;


case 0x4C:


ABS();


JMP();


break;


case 0x4D:


ABS();


EOR();


break;


case 0x4E:


ABS();


LSR();
break;


case 0x50:


REL();


BVC();


break;


case 0x51:


IIY();


EOR();


break;


case 0x55:


ZPX();


EOR();


break;


case 0x56:


ZPX();


LSR();


break;


case 0x58:


CLI();


break;
case 0x59:


ABY();


EOR();


break;


case 0x5D:


ABX();


EOR();


break;


case 0x5E:


ABX();


LSR();


break;


case 0x60:


RTS();
break;
case 0x61:


IIX();


ADC();


break;


case 0x65:


ZDP();
ADC();


break;


case 0x66:


ZDP();


ROR();


break;


case 0x68:


PLA();


break;


case 0x69:


IMM();


ADC();


break;


case 0x6A:


ACC();


ROR();


break;


case 0x6C:


IND();


JMP();
break;


case 0x6D:


ABS();


ADC();


break;


case 0x6E:


ABS();


ROR();


break;


case 0x70:


REL();


BVS();


break;


case 0x71:


IIY();


ADC();


break;


case 0x75:


ZPX();


ADC();
break;


case 0x76:


ZPX();


ROR();


break;


case 0x78:


SEI();


break;


case 0x79:


ABY();


ADC();


break;


case 0x7D:


ABX();


ADC();


break;


case 0x7E:


ABX();


ROR();


break;
case 0x81:


IIX();


STA();


break;


case 0x84:


ZDP();


STY();


break;


case 0x85:


ZDP();


STA();


break;


case 0x86:


ZDP();


STX();


break;


case 0x88:


DEY();


break;


case 0x8A:
TXA();


break;


case 0x8C:


ABS();


STY();


break;


case 0x8D:


ABS();


STA();


break;


case 0x8E:


ABS();


STX();


break;


case 0x90:


REL();


BCC();


break;


case 0x91:


IIY();
STA();


break;


case 0x94:


ZPX();


STY();


break;


case 0x95:


ZPX();


STA();


break;


case 0x96:


ZPY();


STX();


break;


case 0x98:


TYA();


break;


case 0x99:


ABY();


STA();
break;


case 0x9A:


TXS();


break;


case 0x9D:


ABX();


STA();


break;


case 0xA0:


IMM();


LDY();


break;


case 0XA1:


IIX();


LDA();


break;


case 0xA2:


IMM();


LDX();


break;
case 0xA4:


ZDP();


LDY();


break;


case 0xA5:


ZDP();


LDA();


break;


case 0xA6:


ZDP();


LDX();


break;


case 0xA8:


TAY();


break;


case 0xA9:


IMM();


LDA();


break;


case 0xAA:
TAX();


break;


case 0xAC:


ABS();


LDY();


break;


case 0xAD:


ABS();


LDA();


break;


case 0xAE:


ABS();


LDX();


break;


case 0xB0:


REL();


BCS();


break;


case 0xB1:


IIY();
LDA();


break;


case 0xB4:


ZPX();


LDY();


break;


case 0xB5:


ZPX();


LDA();


break;


case 0xB6:


ZPY();


LDX();


break;


case 0xB8:


CLV();


break;


case 0xB9:


ABY();


LDA();
break;


case 0xBA:


TSX();


break;


case 0xBC:


ABX();


LDY();


break;


case 0xBD:


ABX();


LDA();


break;


case 0xBE:


ABY();


LDX();


break;


case 0xC0:


IMM();


CPY();


break;
case 0xC1:


IIX();


CMP();


break;


case 0xC4:


ZDP();


CPY();


break;


case 0xC5:


ZDP();


CMP();


break;


case 0xC6:


ZDP();


DEC();


break;


case 0xC8:


INY();


break;


case 0xC9:
IMM();


CMP();


break;


case 0xCA:


DEX();


break;


case 0xCC:


ABS();


CPY();


break;


case 0xCD:


ABS();


CMP();


break;


case 0xCE:


ABS();


DEC();


break;


case 0xD0:


REL();
BNE();


break;


case 0xD1:


IIY();


CMP();


break;


case 0xD5:


ZPX();


CMP();


break;


case 0xD6:


ZPX();


DEC();


break;


case 0xD8:


CLD();


break;


case 0xD9:


ABY();


CMP();
break;


case 0xDD:


ABX();


CMP();


break;


case 0xDE:


ABX();


DEC();


break;


case 0xE0:


IMM();


CPX();


break;


case 0xE1:


IIX();


SBC();


break;


case 0xE4:


ZDP();


CPX();
break;


case 0xE5:


ZDP();


SBC();


break;


case 0xE6:


ZDP();


INC();


break;


case 0xE8:


INX();


break;


case 0xE9:


IMM();


SBC();


break;


case 0xEA:


NOP();


break;


case 0xEC:
ABS();


CPX();

break;

case 0XED:


ABS();


SBC();


break;


case 0xEE:


ABS();


INC();


break;


case 0xF0:


REL();


BEQ();


break;


case 0xF1:


IIY();


SBC();


break;


case 0xF5:


ZPX();
SBC();


break;


case 0XF6:


ZPX();


INC();


break;


case 0xF8:


SED();


break;


case 0xF9:


ABY();


SBC();


break;


case 0xFD:


ABX();


SBC();


break;


case 0xFE:


ABX();


INC();
break;


}
}

// Espero nunca mas tener que descomentar esto
// char com;
	// int stepsize = 0;
	// bool babalinger = 0;
	// uint16_t breakpoint = 0x338b;
	// while ( !stop ) {
	// 	com = 0;
	// 	if (stepsize) stepsize--;
	// 	if (babalinger) bus->display_section(PC-PC%(DISPLAY_X*DISPLAY_Y), PC);
	// 	execute(bus->read(PC));
	// 	PC++;
	// 	if (PC == 0x338b) {
	// 		PC += 3;
	// 		//babalinger = 1;
	// 	}
	// 	if (PC == 0x33Af) PC += 3;
	// 	if (PC == 0x3469) getchar();
	// 	if (!stepsize&&babalinger) com = getchar();
	// 	if (com == 'p') {
	// 		int dir;
	// 		cin >> hex >> dir;
	// 		bus->display_section(dir);
	// 	} 
	// 	if (PC == breakpoint) {
	// 		stepsize = 0;
	// 		babalinger = 1;
	// 	}
	// 	stepsize += 10*(com == 't');
	// 	stepsize += 100*(com == 'c');
	// 	stepsize += 1000*(com == 'm');
	// 	stepsize += 10000*(com == 'd');
	// 	stop = (com == 'q');
	// 	//cout << "STACK:";
	// 	//for (int i = S; i < 0xFF; i++) cout << " " << hex << unsigned(bus->read(STACK_PAGE | i+1));
	// 	//cout << endl;
	// 	cout << "PC: " << hex << PC << endl;
	// 	if (babalinger) {
	// 			cout << "PC: " << hex << PC << " A: " << hex << unsigned(A) << " X: " << hex << unsigned(X) << " Y: " << hex << unsigned(Y) << endl;
	// 			cout << "N: " << getF(N) << " V: " << getF(V) << " U: " << getF(U) << " B: " << getF(B) << " D: " << getF(D) << " I: " << getF(I) << " Z: " << getF(Z) << " C: " << getF(C) << endl;}
	//}
