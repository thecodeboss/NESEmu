#ifndef APU_h__
#define APU_h__

#include "Global.h"
class CPU;

class APU
{
	uint64 Cycles;
	CPU* cpu;
public:
	APU();
	void Tick();
	void Dump();
	void SetCPU(CPU* c);

	static const uint8 LengthCounters[32];
	static const uint16 NoisePeriods[16];
	static const uint16 DMCperiods[16];

	bool FiveCycleDivider, IRQdisable, ChannelsEnabled[5];
	bool PeriodicIRQ, DMC_IRQ;

	struct { short lo, hi; } hz240counter;

	void Write(uint8 index, uint8 value);
	uint8 Read();
	bool count(int32& v, int32 reset);

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

	// Function for updating the wave generators and taking the sample for each channel.
	template<unsigned c>
	int ChannelTick()
	{
		channel& ch = channels[c];
		if(!ChannelsEnabled[c]) return c==4 ? 64 : 8;
		int wl = (ch.reg.WaveLength+1) * (c >= 2 ? 1 : 2);
		if(c == 3) wl = NoisePeriods[ ch.reg.NoiseFreq ];
		int volume = ch.length_counter ? ch.reg.EnvDecayDisable ? ch.reg.FixedVolume : ch.envelope : 0;
		// Sample may change at wavelen intervals.
		auto& S = ch.level;
		if(!count(ch.wave_counter, wl)) return S;
		switch(c)
		{
		default:// Square wave. With four different 8-step binary waveforms (32 bits of data total).
			if(wl < 8) return S = 8;
			return S = (0xF33C0C04u & (1u << (++ch.phase % 8 + ch.reg.DutyCycle * 8))) ? volume : 0;

		case 2: // Triangle wave
			if(ch.length_counter && ch.linear_counter && wl >= 3) ++ch.phase;
			return S = (ch.phase & 15) ^ ((ch.phase & 16) ? 15 : 0);

		case 3: // Noise: Linear feedback shift register
			if(!ch.hold) ch.hold = 1;
			ch.hold = (ch.hold >> 1)
				| (((ch.hold ^ (ch.hold >> (ch.reg.NoiseType ? 6 : 1))) & 1) << 14);
			return S = (ch.hold & 1) ? 0 : volume;

		case 4: // Delta modulation channel (DMC)
			// hold = 8 bit value, phase = number of bits buffered
			if(ch.phase == 0) // Nothing in sample buffer?
			{
				if(!ch.length_counter && ch.reg.LoopEnabled) // Loop?
				{
					ch.length_counter = ch.reg.PCMlength*16 + 1;
					ch.address        = (ch.reg.reg0 | 0x300) << 6;
				}
				if(ch.length_counter > 0) // Load next 8 bits if available
				{
					// Note: Re-entrant! But not recursive, because even
					// the shortest wave length is greater than the read time.
					// TODO: proper clock
					if(ch.reg.WaveLength>20)
						for(unsigned t=0; t<3; ++t) cpu->Read(uint16(ch.address) | 0x8000); // timing
					ch.hold  = cpu->Read(uint16(ch.address++) | 0x8000); // Fetch byte
					ch.phase = 8;
					--ch.length_counter;
				}
				else // Otherwise, disable channel or issue IRQ
					ChannelsEnabled[4] = ch.reg.IRQenable && (cpu->SetInterrupt(DMC_IRQ = true));
			}
			if(ch.phase != 0) // Update the signal if sample buffer nonempty
			{
				int v = ch.linear_counter;
				if(ch.hold & (0x80 >> --ch.phase)) v += 2; else v -= 2;
				if(v >= 0 && v <= 0x7F) ch.linear_counter = v;
			}
			return S = ch.linear_counter;
		}
	}
};

#endif // APU_h__
