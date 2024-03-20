#pragma once

#include <memory>
#include <stdint.h>
#include <array>

class RNG
{
public:
	RNG();
private:
	std::unique_ptr<double[]> RandomValues;
	size_t RandomValuesSize;
	std::array<uint8_t, 32> RandomValuesSHA256Hash;
	bool bValidRandomValues;
};