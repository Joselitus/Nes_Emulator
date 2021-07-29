nes.o: CPU.o BUS.o PPU.o APU.o ROM.o Mapper.o Display.o CIM.o
	g++ -o test -O3 main.cpp CPU.o PPU.o APU.o ROM.o BUS.o Mapper.o Display.o CIM.o -lSDL2 -lpthread

BUS.o: BUS.h BUS.cpp
	g++ -c -O3 BUS.cpp

CPU.o: CPU.h CPU.cpp
	g++ -c -O3 CPU.cpp

PPU.o: PPU.h PPU.cpp
	g++ -c -O3 PPU.cpp

APU.o: APU.h APU.cpp
	g++ -c -O1 APU.cpp

ROM.o: ROM.h ROM.cpp
	g++ -c -O3 ROM.cpp

Mapper.o: Mapper.h Mapper.cpp
	g++ -c -O3 Mapper.cpp

Display.o: Display.h Display.cpp
	g++ -c -O1 Display.cpp

CIM.o: CIM.h CIM.cpp
	g++ -c -O1 CIM.cpp

clean:
	rm CPU.o BUS.o PPU.o APU.o ROM.o CIM.o Mapper.o Display.o test