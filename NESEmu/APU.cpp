#include "APU.h"
#include "CPU.h"

const uint8 APU::LengthCounters[32] = { 10,254,20, 2,40, 4,80, 6,160, 8,60,10,14,12,26,14,12,16,24,18,48,20,96,22,192,24,72,26,16,28,32,30 };
const uint16 APU::NoisePeriods[16] = { 2,4,8,16,32,48,64,80,101,127,190,254,381,508,1017,2034 };
const uint16 APU::DMCperiods[16] = { 428,380,340,320,286,254,226,214,190,160,142,128,106,84,72,54 };

APU::APU() : Cycles(0), FiveCycleDivider(false), IRQdisable(true), PeriodicIRQ(false), DMC_IRQ(false)
{
	for (int32 i=0; i<5; i++) ChannelsEnabled[i] = false;

	hz240counter.hi = 0;
	hz240counter.lo = 0;
}

void APU::Tick()
{
	Cycles++;
	// Divide CPU clock by 7457.5 to get a 240 Hz, which controls certain events.
	if((hz240counter.lo += 2) >= 14915)
	{
		hz240counter.lo -= 14915;
		if(++hz240counter.hi >= 4+FiveCycleDivider) hz240counter.hi = 0;

		// 60 Hz interval: IRQ. IRQ is not invoked in five-cycle mode (48 Hz).
		if(!IRQdisable && !FiveCycleDivider && hz240counter.hi==0)
			cpu->SetInterrupt(PeriodicIRQ = true);

		// Some events are invoked at 96 Hz or 120 Hz rate. Others, 192 Hz or 240 Hz.
		bool HalfTick = (hz240counter.hi&5)==1, FullTick = hz240counter.hi < 4;
		for(unsigned c=0; c<4; ++c)
		{
			channel& ch = channels[c];
			int wl = ch.reg.WaveLength;

			// Length tick (all channels except DMC, but different disable bit for triangle wave)
			unsigned x = ch.reg.LinearCounterDisable;
			unsigned y = ch.reg.LengthCounterDisable;
			if(HalfTick && ch.length_counter
				&& !(c==2 ? x : y))
				ch.length_counter -= 1;

			// Sweep tick (square waves only)
			if(HalfTick && c < 2 && count(ch.sweep_delay, ch.reg.SweepRate))
				if(wl >= 8 && ch.reg.SweepEnable && ch.reg.SweepShift)
				{
					int s = wl >> ch.reg.SweepShift, d[4] = {s, s, ~s, -s};
					wl += d[ch.reg.SweepDecrease*2 + c];
					if(wl < 0x800) ch.reg.WaveLength = wl;
				}

				// Linear tick (triangle wave only)
				if(FullTick && c == 2)
					ch.linear_counter = ch.reg.LinearCounterDisable
					? ch.reg.LinearCounterInit
					: (ch.linear_counter > 0 ? ch.linear_counter - 1 : 0);

				// Envelope tick (square and noise channels)
				if(FullTick && c != 2 && count(ch.env_delay, ch.reg.EnvDecayRate))
					if(ch.envelope > 0 || ch.reg.EnvDecayLoopEnable)
						ch.envelope = (ch.envelope-1) & 15;
		}
	}

	// Mix the audio: Get the momentary sample from each channel and mix them.
#define s(c) static_cast<float>(ChannelTick<c==1 ? 0 : c>())
	auto v = [](float m,float n, float d) { return n!=0.f ? m/n : d; };
	short sample = static_cast<short>(30000 *
		(v(95.88f,  (100.f + v(8128.f, s(0) + s(1), -100.f)), 0.f)
		+  v(159.79f, (100.f + v(1.0, s(2)/8227.f + s(3)/12241.f + s(4)/22638.f, -100.f)), 0.f)
		- 0.5f
		));
#undef s
	// I cheat here: I did not bother to learn how to use SDL mixer, let alone use it in <5 lines of code,
	// so I simply use a combination of external programs for outputting the audio.
	// Hooray for Unix principles! A/V sync will be ensured in post-process.
	//return; // Disable sound because already device is in use
	//static FILE* fp = popen("resample mr1789800 r48000 | aplay -fdat 2>/dev/null", "w");
	//fputc(sample, fp);
	//fputc(sample/256, fp);
}

void APU::Dump()
{

}

void APU::SetCPU( CPU* c )
{
	cpu = c;
}

uint8 APU::Read()
{
	uint8 res = 0;
	for(unsigned c=0; c<5; ++c) res |= (channels[c].length_counter ? 1 << c : 0);
	if(PeriodicIRQ) res |= 0x40; PeriodicIRQ = false;
	if(DMC_IRQ)     res |= 0x80; DMC_IRQ     = false;
	cpu->SetInterrupt(false);
	return res;
}

void APU::Write( uint8 index, uint8 value )
{
	channel& ch = channels[(index/4) % 5];
	switch(index<0x10 ? index%4 : index)
	{
	case 0: if(ch.reg.LinearCounterDisable) ch.linear_counter=value&0x7F; ch.reg.reg0 = value; break;
	case 1: ch.reg.reg1 = value; ch.sweep_delay = ch.reg.SweepRate; break;
	case 2: ch.reg.reg2 = value; break;
	case 3:
		ch.reg.reg3 = value;
		if(ChannelsEnabled[index/4])
			ch.length_counter = LengthCounters[ch.reg.LengthCounterInit];
		ch.linear_counter = ch.reg.LinearCounterInit;
		ch.env_delay      = ch.reg.EnvDecayRate;
		ch.envelope       = 15;
		if(index < 8) ch.phase = 0;
		break;
	case 0x10: ch.reg.reg3 = value; ch.reg.WaveLength = DMCperiods[value&0x0F]; break;
	case 0x12: ch.reg.reg0 = value; ch.address = (ch.reg.reg0 | 0x300) << 6; break;
	case 0x13: ch.reg.reg1 = value; ch.length_counter = ch.reg.PCMlength*16 + 1; break; // sample length
	case 0x11: ch.linear_counter = value & 0x7F; break; // dac value
	case 0x15:
		for(unsigned c=0; c<5; ++c)
			ChannelsEnabled[c] = (value & (1 << c)) != 0;
		for(unsigned c=0; c<5; ++c)
			if(!ChannelsEnabled[c])
				channels[c].length_counter = 0;
			else if(c == 4 && channels[c].length_counter == 0)
				channels[c].length_counter = ch.reg.PCMlength*16 + 1;
		break;
	case 0x17:
		IRQdisable       = (value & 0x40) != 0;
		FiveCycleDivider = (value & 0x80) != 0;
		hz240counter.hi = 0;
		hz240counter.lo = 0;
		if(IRQdisable) PeriodicIRQ = DMC_IRQ = false;
	}
}

bool APU::count( int32& v, int32 reset )
{
	return --v < 0 ? (v=reset),true : false;
}