#include "SN76489.h"

struct ToneGenerator
{
	inline short Frequency()
	{
		return f;
	}
	inline char Attenuation()
	{
		return a;
	}
	inline void Attenuation(char newval)
	{
		a = newval & 0xf;
	}
	inline void Frequency(short newval)
	{
		f = newval & 0x3ff;	
	}
	int AttenuationDb()
	{
		return a == 0xf ? 0 : ((a & 1) * 2)
			+ ((a & (1 << 1)) * 4)
			+ ((a & (1 << 2)) * 8)
			+ ((a & (1 << 3)) * 16);
	}
	inline bool Update()
	{
		return o;
	}
	inline void Level(bool val)
	{
		o = val;
	}

private:
	unsigned short f : 10;
	unsigned short a : 4;
	unsigned short o : 1;
};

enum NoiseType
{
	PeriodicNoise = 0,
	WhiteNoise,
};

enum ShiftRate
{
	N512,
	N1024,
	N2048,
	Tone3Output
};

struct NoiseGenerator
{
public:
	inline NoiseType Type()
	{
		(NoiseType)fb;
	}
	void Type(NoiseType t)
	{
		nf = t;
	}

	inline ShiftRate Rate()
	{

	}

private:
	unsigned char  fb : 1;
	unsigned char nf : 2;
};





struct SN76489
{
	ToneGenerator toneGenerators[3];
};