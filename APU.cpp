#include "APU.h"

APU::APU(int volume) {
	amplitude = volume;
	elapsed = 0.0;
	counter = 0;
	buffer_read = buffer_write = 0;
	current_cycle = 0;
	v = 0;
	frq = 0;
	pulse1_seq = new Sequencer(1);
	pulse2_seq = new Sequencer(0);
	triang_seq = new Sequencer(0);


	uint8_t lt[] = {  10, 254, 20,  2, 40,  4, 80,  6,
			         160,   8, 60, 10, 14, 12, 26, 14,
			          12,  16, 24, 18, 48, 20, 96, 22,
			         192,  24, 72, 26, 16, 28, 32, 30 };
	for (int i = 0; i < 8*4; i++)
		length_table[i] = lt[i];
}

void APU::tick() {
	//std::cout << "nw: " << buffer_write << ", nr: " << buffer_read << std::endl;
	counter = (counter+1)%6;
	if (!counter) {
		bool QFrame = false;
		bool HFrame = false;
		current_cycle++;

		if (current_cycle == 3729) QFrame = true;
		if (current_cycle == 7457) QFrame = HFrame = true;
		if (current_cycle == 11186) QFrame = true;
		if (current_cycle == 14916) {
			QFrame = HFrame = 0;
			current_cycle = 0;
		}

		if (QFrame) {
			pulse1_seq->tick_env();
			pulse2_seq->tick_env();
			triang_seq->tick_li_cnt();
		}
		if (HFrame) {
			pulse1_seq->tick_cnt();
			pulse1_seq->tick_swp();
			pulse2_seq->tick_cnt();
			pulse2_seq->tick_swp();
			triang_seq->tick_cnt();
			// std::cout << pulse2_seq->getDct() << std::endl;
			// std::cout << pulse1_seq->getDct() << std::endl;

		}

		pulse1_seq->tick([](uint32_t &x) {
				x = (((x&0x0001)<<7) | ((x&0x00FE)>>1));
			}
		);
		pulse2_seq->tick([](uint32_t &x) {
				x = (((x&0x0001)<<7) | ((x&0x00FE)>>1));
			}
		);
	}

	triang_seq->tick([](uint32_t &x) {
			// TODO
			x = 0x0000;
		}
	);

	pulse1_seq->track();
	pulse2_seq->track();

	elapsed += time_per_ppu_cycle;
	if (elapsed > time_per_sample) {
		elapsed -= time_per_sample;
		generateSample();
	}
}

void APU::write(uint16_t addr, uint8_t data) {
	switch (addr) {
		case 0x4000:
			switch ((data&0xC0) >> 6) {
				case 0x00: 
					pulse1_seq->setSeq(0x00000001);
					pulse1_seq->setDut(0.125);
				break;
				case 0x01: 
					pulse1_seq->setSeq(0x00000011);
					pulse1_seq->setDut(0.250);
				break;
				case 0x02: 
					pulse1_seq->setSeq(0x00001111);
					pulse1_seq->setDut(0.500);
				break;
				case 0x03: 
					pulse1_seq->setSeq(0x11111100);
					pulse1_seq->setDut(0.275);
				break;
			}
			pulse1_seq->setHlt(data&0x20);
			pulse1_seq->setVlm(data&0x0F);
			pulse1_seq->setDis(data&0x10);
		break;
		case 0x4001:
			pulse1_seq->setSpEn(data&0x80);
			pulse1_seq->setPr((data&0x70)>>4);
			pulse1_seq->setDn(data&0x08);
			pulse1_seq->setSh(data&0x07);
			pulse1_seq->setRst(true);
		break;
		case 0x4002:
			pulse1_seq->setRelUp(data);
		break;
		case 0x4003:
			pulse1_seq->setRelDn(data);
			pulse1_seq->setDct(length_table[(data&0xF8)>>3]);
			pulse1_seq->setStr(true);
		break;
		case 0x4004:
			switch ((data&0xC0) >> 6) {
				case 0x00: 
					pulse2_seq->setSeq(0x00000001);
					pulse2_seq->setDut(0.125);
				break;
				case 0x01: 
					pulse2_seq->setSeq(0x00000011);
					pulse2_seq->setDut(0.250);
				break;
				case 0x02: 
					pulse2_seq->setSeq(0x00001111);
					pulse2_seq->setDut(0.500);
				break;
				case 0x03: 
					pulse2_seq->setSeq(0x11111100);
					pulse2_seq->setDut(0.275);
				break;
			}
			pulse2_seq->setHlt(data&0x20);
			pulse2_seq->setVlm(data&0x0F);
			pulse2_seq->setDis(data&0x10);
		break;
		case 0x4005:
			pulse2_seq->setSpEn(data&0x80);
			pulse2_seq->setPr((data&0x70)>>4);
			pulse2_seq->setDn(data&0x08);
			pulse2_seq->setSh(data&0x07);
			pulse2_seq->setRst(true);
		break;
		case 0x4006:
			pulse2_seq->setRelUp(data);
		break;
		case 0x4007:
			pulse2_seq->setRelDn(data);
			pulse2_seq->setDct(length_table[(data&0xF8)>>3]);
			pulse2_seq->setStr(true);
		break;
		case 0x4008:
			triang_seq->setLiCnt(data&0x80);
			triang_seq->setLiRel(data&0x7F);
		break;
		case 0x400A:
			triang_seq->setRelUp(data);
		break;
		case 0x400B:
			triang_seq->setRelDn(data);
			triang_seq->setDct(length_table[(data&0xF8)>>3]);
			triang_seq->setLiRst(true);
		case 0x400C:
		break;
		case 0x400E:
		break;
		case 0x400F:
			pulse1_seq->setStr(true);
			pulse2_seq->setStr(true);
		break;
		case 0x4015:
			pulse1_seq->setEn(data&0x01);
			pulse2_seq->setEn(data&0x02);
			triang_seq->setEn(data&0x04);
		break;
	}
}

uint8_t APU::read(uint16_t addr) {
	switch (addr) {
		case 0x4015:
			uint8_t cnt1 = pulse1_seq->getDct() ? 0x01 : 0x00;
			uint8_t cnt2 = pulse2_seq->getDct() ? 0x02 : 0x00;
			return cnt1 | cnt2;
		break;
	}
	return 0;
}

float apsin(float v) {
	float j = v * 0.15915;
	j = j - (int)j;
	return 20.785 * j * (j-0.5) * (j-1.0f);
}

// float apsin(float x) {
// 	return sin(x);
// }

void APU::generateSample() {
	Sint16 pulseval = 0.0;
	Sequencer * seq = pulse1_seq;
	for (int i = 0; i < 2; i++) {
		if (seq->getDct() && seq->getEn() && (seq->getCnt() >= 8) && (seq->getEnv() > 2) && (!seq->getMut())) {
			float p = seq->getDut() * 2.0 * PI;
			float a = 0.0;
			float b = 0.0;
			double v = seq->getTime();
			//#pragma omp parallel for shared(a, b) reduction(+: a, b)
			for (float j = 1.0; j < 50; j++) {
				float c = (j*2.0*PI*v/FREQUENCY);
				a += -apsin(c)/j;
				b += -apsin(c-p*j)/j;
			}
			pulseval += amplitude*seq->getAmp()*(2.0/PI)*(a-b);
		}
		else pulseval += 0.0;
		seq = pulse2_seq;
	}
	Sint16 trival = 0.0;
	if (triang_seq->getEn() && triang_seq->getDct() && triang_seq->getLiCnt()) {
		v = triang_seq->getTimeTri();
		float a = 0.0;
		//#pragma omp parallel for shared(a) reduction(+: a)
		for (int i = 0; i < 10; i++) {
			float n = 2*i + 1;
			a += (1/(n*n))*apsin(2.0*PI*n*v/FREQUENCY) * (i%2 ? -1 : 1);
		}
		trival = amplitude*(8.0/(PI*PI))*a;
	}
	else trival = 0.0;
	
	while (!bufferWrite(pulseval+trival));
}

// Audio sample generation
void APU::getSamples(Sint16 *stream, int length) {
	Sint16 readval;
	for (int i = 0; i < length; i++) {
		while(!bufferRead(&readval));
		//	std::cout << "read: " << readval << std::endl;
		stream[i] = readval;
	}
}

bool APU::bufferWrite(Sint16 value) {
	if ((buffer_write+1)%BUFFER_SIZE == buffer_read)
		return false;
	audio_buffer[buffer_write] = value;
	buffer_write = (buffer_write+1)%BUFFER_SIZE;
	return true;
}

bool APU::bufferRead(Sint16 * sample) {
	if (buffer_read+1 == buffer_write || buffer_read == buffer_write)
		return false;
	*sample = audio_buffer[buffer_read];
	buffer_read = (buffer_read+1)%BUFFER_SIZE;
	return true;
}
