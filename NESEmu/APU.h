#ifndef APU_h__
#define APU_h__

#include "Global.h"
#include "IO.h"
class CPU;

class APU
{
	uint64 Cycles;
	IO* io;
	CPU* cpu;

	static const uint8 LengthCounters[32];
	static const uint16 NoisePeriods[16];
	static const uint16 DMCperiods[16];

	bool FiveCycleDivider, IRQdisable, ChannelsEnabled[5];
	bool PeriodicIRQ, DMC_IRQ;
	bool count(int& v, int reset);

	struct channel
	{
		int length_counter, linear_counter, address, envelope;
		int sweep_delay, env_delay, wave_counter, hold, phase, level;
		union // Per-channel register file
		{
			// 4000, 4004, 400C, 4012:            // 4001, 4005, 4013:            // 4002, 4006, 400A, 400E:
			Bit<0,8,uint32> reg0;                 Bit< 8,8,uint32> reg1;          Bit<16,8,uint32> reg2;
			Bit<6,2,uint32> DutyCycle;            Bit< 8,3,uint32> SweepShift;    Bit<16,4,uint32> NoiseFreq;
			Bit<4,1,uint32> EnvDecayDisable;      Bit<11,1,uint32> SweepDecrease; Bit<23,1,uint32> NoiseType;
			Bit<0,4,uint32> EnvDecayRate;         Bit<12,3,uint32> SweepRate;     Bit<16,11,uint32> WaveLength;
			Bit<5,1,uint32> EnvDecayLoopEnable;   Bit<15,1,uint32> SweepEnable;   // 4003, 4007, 400B, 400F, 4010:
			Bit<0,4,uint32> FixedVolume;          Bit< 8,8,uint32> PCMlength;     Bit<24,8,uint32> reg3;
			Bit<5,1,uint32> LengthCounterDisable;                                 Bit<27,5,uint32> LengthCounterInit;
			Bit<0,7,uint32> LinearCounterInit;                                    Bit<30,1,uint32> LoopEnabled;
			Bit<7,1,uint32> LinearCounterDisable;                                 Bit<31,1,uint32> IRQenable;
		} reg;
	} channels[5];

	struct { short lo, hi; } hz240counter;

public:
	APU();
	void Tick();
	void Dump();
	void SetCPU(CPU* c);
	void SetIO(IO* i);
	void Write(uint8 index, uint8 value);
	uint8 Read();
	int TickChannel(channel& ch, uint32 c);
};

#endif // APU_h__
