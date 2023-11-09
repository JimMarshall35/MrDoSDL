#pragma once

enum class EnemyType
{
	Normal,
	TurningIntoDigger,
	Digger,
	ExtraMan,
	Ghost
};

inline bool IsSignificantEnemyType(EnemyType t)
{
	return t == EnemyType::Normal || t == EnemyType::TurningIntoDigger || t == EnemyType::Digger;
}