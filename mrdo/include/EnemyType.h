#pragma once

enum class EnemyType : uint32_t
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