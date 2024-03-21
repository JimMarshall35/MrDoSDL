#pragma once

#include <memory>
#include <stdint.h>
#include <array>
#include <random>

class IRNG
{
	virtual double Get0To1() = 0;
	virtual unsigned int GetSeed() = 0;
};

class RNG : public IRNG
{
public:
	RNG();
	RNG(uint32_t seed);
	virtual double Get0To1() override;
	virtual unsigned int GetSeed() override
	{
		return Seed;
	}
private:
	void InitRNGBuffer();
private:
	std::vector<double> RandomValues;
	std::mt19937 MTwister;
	size_t NextIndex = 0;
	unsigned int Seed;
};