#pragma once

enum class CharacterDeathReason
{
	CrushedByApple,
	KilledByMonster
};

struct CharacterDied
{
	CharacterDeathReason Reason;
};