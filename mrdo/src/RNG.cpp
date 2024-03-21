#include "RNG.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include "SHA256.h"
#include <vector>

#define BUFFER_SIZE 100000
RNG::RNG()
{
	MTwister.seed(time(NULL));
	InitRNGBuffer();
}

RNG::RNG(uint32_t seed)
{
	MTwister.seed(seed);
	InitRNGBuffer();
}

double RNG::Get0To1()
{
	double r = RandomValues[NextIndex++];
	if (NextIndex >= BUFFER_SIZE)
	{
		NextIndex = 0;
	}
	return r;
}

void RNG::InitRNGBuffer()
{
	RandomValues.resize(BUFFER_SIZE);
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		RandomValues[i] = MTwister() / MTwister.max();
	}
}
