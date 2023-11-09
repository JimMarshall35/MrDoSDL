#pragma once
#include "VectorTypes.h"
#include "EnemyType.h"

enum class CharacterDeathReason
{
	CrushedByApple,
	KilledByMonster
};

struct CharacterDied
{
	CharacterDeathReason Reason;
};

struct CherryEaten
{
	ivec2 Location;
};

enum class EnemyDeathReason
{
	Apple,
	CrystalBall
};

struct EnemyDeath
{
	EnemyDeathReason Reason;
	u16 NumberKilledTotal;
	u16 NumberSignificantKilled;
};

enum class VictoryReason
{
	Cherries,
	Monsters
};

struct Victory
{
	VictoryReason Reason;
};

struct GameOver{};