#include "RNG.h"
#include <iostream>
#include <fstream>
#include <random>
#include <ctime>
#include "SHA256.h"
#include <vector>

#define FileName "RNG.bin"

double Randomnumber() {
	// Making rng static ensures that it stays the same
	// Between different invocations of the function
	static std::default_random_engine rng;
	rng.seed(time(NULL));

	std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(rng);
}


void GenerateRNGFile(int size)
{
	std::ofstream os(FileName);
	for (int i = 0; i < size; i++)
	{
		double d = Randomnumber();
		os << d;
	}
	

}

void PrintSha(const std::array<uint8_t, 32>& sha, const std::string varName = "sExpectedSHA256")
{
	std::cout << "std::array<uint8_t, 32> "<< varName << " = { ";
	for (int i = 0; i < 32; i++)
	{
		printf("0x%.2X, ", sha[i]);

	}
	std::cout << "};\n";
}

std::unique_ptr<double[]> LoadRNGFile(int outBufSize)
{
	auto outBuf = std::make_unique<double[]>(outBufSize);
	std::ifstream is(FileName);
	double num;
	int i = 0;
	while (is >> num)
	{
		outBuf[i++] = num;
	}

	return outBuf;
}
std::array<uint8_t, 32> sExpectedSHA256 = { 0xF1, 0xFF, 0xA2, 0xEC, 0x28, 0xCA, 0x18, 0xF3, 0xEE, 0x01, 0x95, 0x4E, 0x4A, 0x75, 0xB9, 0x5F, 0x8B, 0x19, 0x91, 0x03, 0xB2, 0xDD, 0xAC, 0x07, 0x68, 0xC6, 0xE2, 0xF9, 0xDD, 0xE1, 0x51, 0x73, };

bool DoesCalculatedHashMatchExpected(const std::array<uint8_t, 32>& calculated)
{
	for (int i = 0; i < 32; i++)
	{
		if (calculated[i] != sExpectedSHA256[i])
		{
			return false;
		}
	}
	return true;
}

RNG::RNG()
{
	//GenerateRNGFile(1000000 / sizeof(double));
	RandomValuesSize = 1000000 / sizeof(double);
	RandomValues = LoadRNGFile(RandomValuesSize);
	SHA256 sha;
	sha.update((const uint8_t*)RandomValues.get(), sizeof(double) * RandomValuesSize);
	RandomValuesSHA256Hash = sha.digest();
	PrintSha(RandomValuesSHA256Hash);
	bValidRandomValues = DoesCalculatedHashMatchExpected(RandomValuesSHA256Hash);


}
